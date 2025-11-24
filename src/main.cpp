#include <Arduino.h>
#include <Wire.h>

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include <AccelStepper.h>
#include <FastLED.h>
#include <Adafruit_SSD1306.h>

#include "HC_SR04.h"

// AP-Setup
const char* ssid = "ESP32-Roboter";
const char* password = "12345678";
AsyncWebServer server(80);

#define BTN_IN    21

#define UltraSonicTX  20
#define UltraSonicRX  10

#define MOTOR1_1  3
#define MOTOR1_2  0
#define MOTOR1_3  1
#define MOTOR1_4  4
#define MOTOR2_1  5
#define MOTOR2_2  7
#define MOTOR2_3  6
#define MOTOR2_4  8

bool b_buttonState;
long l_distance;
String APIPAdress;

//  Steppers
const float Motor_Speed = 750.0;
const float Motor_Acc = 750.0;
AccelStepper Motor1(AccelStepper::FULL4WIRE, MOTOR1_1, MOTOR1_2, MOTOR1_3, MOTOR1_4);
AccelStepper Motor2(AccelStepper::FULL4WIRE, MOTOR2_1, MOTOR2_2, MOTOR2_3, MOTOR2_4);
String currentDir = "stop";

//Display
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

//Ultrasonic
HC_SR04 Ultrasonic(UltraSonicTX, UltraSonicRX);

void handleOLED() {
  display.clearDisplay();
  display.setRotation(2);       // Use full 256 char 'Code Page 437' font
  display.cp437(true); 
  display.setTextSize(2);       // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(35, 5);     // Start at top-left corner
  display.println(F("Happy"));
  display.setCursor(35, 25);
  display.println(F("B-Day!"));
  display.setCursor(35, 50);
  display.setTextSize(1);
  display.print(APIPAdress);
  display.display();
}

////////////////////////////////////
////                            ////
////           SETUP            ////
////                            ////
////////////////////////////////////

void setup() {

  Serial.begin(115200);
  Serial.println("Serial:         OK!");
 
  if(!LittleFS.begin()){
    Serial.println("LittleFS Error!");
    return;
  }
  Serial.println("LittleFS:       OK!");

  WiFi.softAP(ssid, password);
  Serial.print("AP: OK, IP: ");
  APIPAdress = WiFi.softAPIP().toString();
  Serial.println(APIPAdress);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/index.html", "text/html");
  });

  server.on("/move", HTTP_GET, [](AsyncWebServerRequest *request){
    if(request->hasParam("dir")){
      String dir = request->getParam("dir")->value();
      currentDir = dir;
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing dir");
    }
  });

  server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", String(l_distance));
  });

  server.begin();

  Wire.begin(2, 9);
  Wire.setClock(400000);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Serial.println("I2C/ Display:   OK!");

  pinMode(BTN_IN, INPUT);
  Serial.println("IO:             OK!");

  Motor1.setMaxSpeed(Motor_Speed);
  Motor1.setAcceleration(Motor_Acc);
  Motor2.setMaxSpeed(Motor_Speed);
  Motor2.setAcceleration(Motor_Acc);
  Serial.println("Motors:         OK!");

  Ultrasonic.begin();
  delay(25);
  Ultrasonic.start();

  handleOLED();

// END OF SETUP
}

////////////////////////////////////
////                            ////
////             MAIN           ////
////                            ////
////////////////////////////////////

void loop() {

  long step = 500;

  EVERY_N_MILLISECONDS(10000)
  {   
    handleOLED();
  }

  EVERY_N_MILLISECONDS(500)
  {
    Serial.println(currentDir);
    if(Ultrasonic.isFinished()){
      l_distance = Ultrasonic.getRange();
      Ultrasonic.start();
    }
  }

  Motor1.enableOutputs();
  Motor2.enableOutputs();
  if(currentDir  == "forward") {
    Motor1.move(Motor1.currentPosition() + step);
    Motor2.move(Motor2.currentPosition() - step);
   }
  else if(currentDir  == "backward") {
    Motor1.move(Motor1.currentPosition() - step);
    Motor2.move(Motor2.currentPosition() + step);
   }
  else if(currentDir  == "left") {
    Motor1.move(Motor1.currentPosition() + step);
    Motor2.move(Motor2.currentPosition() + step);
  }
  else if(currentDir  == "right") {
    Motor1.move(Motor1.currentPosition() - step);
    Motor2.move(Motor2.currentPosition() - step);
  }
  else { 
    Motor1.setCurrentPosition(0);
    Motor2.setCurrentPosition(0);
    Motor1.move(0);
    Motor2.move(0); 
  }

  Motor1.run(); 
  Motor2.run();

  if (!Motor1.isRunning() && !Motor2.isRunning())
  {
    Motor1.disableOutputs();
    Motor2.disableOutputs();
  }

// END OF LOOP
}
