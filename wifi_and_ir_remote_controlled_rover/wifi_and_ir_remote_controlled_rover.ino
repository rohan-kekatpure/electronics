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

// Rover constants
enum class Side {LEFT, RIGHT};
enum class Direction {FWD, REV};

const uint PWM = 255;
double GLOBAL_LEFT_SPEED = 100.0;
double GLOBAL_RIGHT_SPEED = 100.0;

const uint8_t LEFT_PWM = D0;
const uint8_t LEFT_IN1 = D1;
const uint8_t LEFT_IN2 = D2;

const uint8_t RIGHT_PWM = D3;
const uint8_t RIGHT_IN1 = D4;
const uint8_t RIGHT_IN2 = D5;

const uint8_t LEFT_INDICATOR = D7;
const uint8_t RIGHT_INDICATOR = D8;

// Webserver constants
const char* SSID = "SSID";
const char* PASSWD = "PASSWD";

// IR Remote control constants 
const uint16_t IR_RECV_PIN = D6;
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
  analogWrite(LEFT_PWM, value);
}

void setRightSpeed(const double value) {
  analogWrite(RIGHT_PWM, value);
}

void setSpeed(const double leftSpeed, const double rightSpeed) {
  setLeftSpeed(leftSpeed);
  setRightSpeed(rightSpeed);
}

void setDirection(Side side, Direction dir) {  
  // Car has only two sides
  if ((side != Side::LEFT) && (side != Side::RIGHT)) {    
    return;
  }

  // There are only two possible directions
  if ((dir != Direction::FWD) && (dir != Direction::REV)) {
    return;
  }

  uint8_t IN1, IN2;
  if (side == Side::LEFT) {
    IN1 = LEFT_IN1;
    IN2 = LEFT_IN2;
  } else {
    IN1 = RIGHT_IN1;
    IN2 = RIGHT_IN2;
  }

  if (dir == Direction::FWD) {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, HIGH);
  } else {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
  }
}

void leftFwd() {
  setDirection(Side::LEFT, Direction::FWD);
}

void leftReverse() {
  setDirection(Side::LEFT, Direction::REV);
}

void rightFwd() {
  setDirection(Side::RIGHT, Direction::FWD);
}

void rightReverse() {
  setDirection(Side::RIGHT, Direction::REV);
}

void fwd() {
  leftFwd();
  rightFwd();
}

void reverse() {
  leftReverse();
  rightReverse();
}

void turnIndicatorOn(const uint8_t indicatorPin) {
  digitalWrite(indicatorPin, HIGH);
}

void turnIndicatorOff(const uint8_t indicatorPin) {
  digitalWrite(indicatorPin, LOW);
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

void turnLeft() {
  turnIndicatorOn(LEFT_INDICATOR);
  leftReverse();
  delay(500);
  leftFwd();  
  turnIndicatorOff(LEFT_INDICATOR);
}

void turnRight() {
  turnIndicatorOn(RIGHT_INDICATOR);
  rightReverse();
  delay(500);
  rightFwd();
  turnIndicatorOff(RIGHT_INDICATOR);
}

void IRSetSpeed(uint level) {
  if ((level < IR_MIN_LEVEL) || (level > IR_MAX_LEVEL)) {
    return;
  }

  double speed = ceil(MIN_SPEED + level * LEVEL2SPEED); 
  GLOBAL_LEFT_SPEED = speed;
  GLOBAL_RIGHT_SPEED = speed; 
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
      turnLeft();
      break;
    
    // Right turn
    case IRCODES::LG_BTN_RIGHT_ARROW:
    case IRCODES::ELEGOO_BTN_RIGHT:
      Serial.println("BUTTON RIGHT");
      turnRight();
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
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(LEFT_IN1, OUTPUT);
  pinMode(LEFT_IN2, OUTPUT);

  pinMode(RIGHT_PWM, OUTPUT);
  pinMode(RIGHT_IN1, OUTPUT);
  pinMode(RIGHT_IN2, OUTPUT);

  pinMode(LEFT_INDICATOR, OUTPUT);
  pinMode(RIGHT_INDICATOR, OUTPUT);

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
  server.handleClient();
  handleIR();
}
