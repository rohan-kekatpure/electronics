#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>

#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "ircodes.h"

// Webserver constants
const char* SSID = "ssid";
const char* PASSWD = "passwd";
const uint PWM = 255;
double GLOBAL_LEFT_SPEED = 100.0;
double GLOBAL_RIGHT_SPEED = 100.0;
const uint8_t LEFT_SIDE_CONTROL_PIN = D1;
const uint8_t RIGHT_SIDE_CONTROL_PIN = D2;

// IR Remote control constants 
const uint16_t IR_RECV_PIN = D3;
IRrecv irrecv(IR_RECV_PIN);
decode_results IR_RESULT;
const uint IR_MIN_LEVEL = 0;
const uint IR_MAX_LEVEL = 9;
const double MIN_SPEED = 0;
const double MAX_SPEED = 255;
const double LEVEL2SPEED = double(MAX_SPEED - MIN_SPEED) / double(IR_MAX_LEVEL - IR_MIN_LEVEL);  
const double SPEED_DELTA = LEVEL2SPEED;


ESP8266WebServer server(80);

void setLeftSpeed(const double value) {
  analogWrite(LEFT_SIDE_CONTROL_PIN, value);
}

void setRightSpeed(const double value) {
  analogWrite(RIGHT_SIDE_CONTROL_PIN, value);
}

void setSpeed(const double leftSpeed, const double rightSpeed) {
  setLeftSpeed(leftSpeed);
  setRightSpeed(rightSpeed);
}

void serveIndex() {
  if (! LittleFS.begin()) {
    Serial.println("Error mounting LittleFS");    
  }
  server.serveStatic("/", LittleFS, "/index.html");  
}

void handlexy() {
  JSONVar coords = JSON.parse(server.arg(0));
  
  if (JSON.typeof(coords) == "undefined"){
    Serial.println(server.arg(0));
    Serial.println("Failed to parse request payload");
    return;
  }

  // Received from the front end
  double x = coords["x"];
  double y = coords["y"];

  // Derived parameters
  double r = sqrt(x * x + y * y);
  double theta_rad = PI / 2.0 - atan2(y, x);
  double theta_deg = theta_rad * RAD_TO_DEG;

  // Conversion to motor PWMs
  double base_speed = r; // speed is distance from the center
  double left_speed = base_speed;
  double right_speed = base_speed;
  double factor = 1.0 - fabs(sin(theta_rad));

  if ((theta_deg >= 0.0) && (theta_deg <= 90.0)) {
    right_speed *= factor;    
  } else if ((theta_deg < 0.0) && (theta_deg >= -90.0)) {
    left_speed *= factor;
  }

  // Convert speeds to PWM
  left_speed *= PWM;
  right_speed *= PWM; 

  // Change speed state
  GLOBAL_LEFT_SPEED = left_speed;
  GLOBAL_RIGHT_SPEED = right_speed;  

  String fmt = "LEFT_SPEED = %0.2f, RIGHT_SPEED = %0.2f\n";
  Serial.printf(fmt.c_str(), GLOBAL_LEFT_SPEED, GLOBAL_RIGHT_SPEED);

  // Write to Drive pins
  setSpeed(GLOBAL_LEFT_SPEED, GLOBAL_RIGHT_SPEED);
}

void IRLeftTurn() {
  setLeftSpeed(MIN_SPEED);
  delay(500);
  setLeftSpeed(GLOBAL_LEFT_SPEED);
}

void IRRightTurn() {
  setRightSpeed(MIN_SPEED);
  delay(500);
  setRightSpeed(GLOBAL_RIGHT_SPEED);
}

void IRSetSpeed(uint level) {
  if ((level < IR_MIN_LEVEL) || (level > IR_MAX_LEVEL)) {
    return;
  }

  double speed = ceil(MIN_SPEED + level * LEVEL2SPEED);  
  setSpeed(speed, speed);
}

void IRStopCar() {
  IRSetSpeed(0);
}

void IRIncrSpeed() {
  double newSpeed = GLOBAL_LEFT_SPEED + SPEED_DELTA;
  GLOBAL_LEFT_SPEED = min(MAX_SPEED, newSpeed);
  GLOBAL_RIGHT_SPEED = GLOBAL_LEFT_SPEED;
  setSpeed(GLOBAL_LEFT_SPEED, GLOBAL_RIGHT_SPEED);
}

void IRDecrSpeed() {
  double newSpeed = GLOBAL_LEFT_SPEED - SPEED_DELTA;
  GLOBAL_LEFT_SPEED = max(MIN_SPEED, newSpeed);
  GLOBAL_RIGHT_SPEED = GLOBAL_LEFT_SPEED;
  setSpeed(GLOBAL_LEFT_SPEED, GLOBAL_RIGHT_SPEED);
}

void handleIR() {
  if (! irrecv.decode(&IR_RESULT)) { 
    return;
  }   
  switch (IR_RESULT.value) {

    // Left turn
    case IRCODES::LG_BTN_LEFT_ARROW:
    case IRCODES::ELEGOO_BTN_LEFT:
      Serial.println("BUTTON LEFT");
      IRLeftTurn();
      break;
    
    // Right turn
    case IRCODES::LG_BTN_RIGHT_ARROW:
    case IRCODES::ELEGOO_BTN_RIGHT:
      Serial.println("BUTTON RIGHT");
      IRRightTurn();
      break;

    // Increase speed
    case IRCODES::LG_BTN_UP_ARROW:
    case IRCODES::ELEGOO_BTN_UP:
      Serial.println("BUTTON UP");
      IRIncrSpeed();
      break;
    
    // Decrease speed
    case IRCODES::LG_BTN_DOWN_ARROW:
    case IRCODES::ELEGOO_BTN_DOWN:
      Serial.println("BUTTON DOWN");
      IRDecrSpeed();
      break;

    // Stop the car
    case IRCODES::LG_BTN_OK:
    case IRCODES::ELEGOO_BTN_OK:
      Serial.println("BUTTON OK");
      IRStopCar();
      break;

    // Speed setting 1
    case IRCODES::LG_BTN_1:
    case IRCODES::ELEGOO_BTN_1:
      Serial.println("BUTTON 1");
      IRSetSpeed(1);
      break;

    // Speed setting 2
    case IRCODES::LG_BTN_2:
    case IRCODES::ELEGOO_BTN_2:
      Serial.println("BUTTON 2");
      IRSetSpeed(2);
      break;

    // Speed setting 3
    case IRCODES::LG_BTN_3:
    case IRCODES::ELEGOO_BTN_3:
      Serial.println("BUTTON 3");
      IRSetSpeed(3);
      break;

    // Speed setting 4
    case IRCODES::LG_BTN_4:
    case IRCODES::ELEGOO_BTN_4:
      Serial.println("BUTTON 4");
      IRSetSpeed(4);
      break;

    // Speed setting 5
    case IRCODES::LG_BTN_5:
    case IRCODES::ELEGOO_BTN_5:
      Serial.println("BUTTON 5");
      IRSetSpeed(5);
      break;

    // Speed setting 6
    case IRCODES::LG_BTN_6:
    case IRCODES::ELEGOO_BTN_6:
      Serial.println("BUTTON 6");
      IRSetSpeed(6);
      break;

    // Speed setting 7
    case IRCODES::LG_BTN_7:
    case IRCODES::ELEGOO_BTN_7:
      Serial.println("BUTTON 7");
      IRSetSpeed(7);
      break;

    // Speed setting 8
    case IRCODES::LG_BTN_8:
    case IRCODES::ELEGOO_BTN_8:
      Serial.println("BUTTON 8");
      IRSetSpeed(8);
      break;

    // Speed setting 9
    case IRCODES::LG_BTN_9:
    case IRCODES::ELEGOO_BTN_9:
      Serial.println("BUTTON 9");
      IRSetSpeed(9);
      break;

    // Stop the car
    case IRCODES::LG_BTN_0:
    case IRCODES::ELEGOO_BTN_0:
      Serial.println("BUTTON 0");
      IRStopCar();
      break;

    // Stop the car
    case IRCODES::ELEGOO_BTN_STAR:
      Serial.println("BUTTON STAR");
      IRStopCar();
      break;

    // Stop the car
    case IRCODES::LG_BTN_POWER:
    case IRCODES::ELEGOO_BTN_HASH:
      Serial.println("BUTTON HASH");
      IRStopCar();
      break;

    default:
      Serial.println("OTHER or UNKNOWN BUTTON");
      break;
  }  
  irrecv.resume();  // Receive the next value
  delay(100);  
}

void setup() {     
  // Pin Setup  
  pinMode(LEFT_SIDE_CONTROL_PIN, OUTPUT);
  pinMode(RIGHT_SIDE_CONTROL_PIN, OUTPUT);

  Serial.begin(115200);

  // Connect Wifi
  Serial.printf("Connecting to: %s\n", SSID);
  WiFi.begin(SSID, PASSWD);  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected");
  Serial.println(WiFi.localIP()); 

  // Setup of IR remote control
  irrecv.enableIRIn();  

  // Start Webserver
  serveIndex();
  server.on("/xy", handlexy);
  server.begin();  
}

void loop() {
  // server.handleClient();
  handleIR();
}
