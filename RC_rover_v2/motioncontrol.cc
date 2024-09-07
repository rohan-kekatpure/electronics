#include<Arduino.h>
#include "pinout.h"
#include "motioncontrol.h"

MotionControl::MotionControl(const PINS& pins): pins{pins} {} 

void MotionControl::setLeftSpeed(uint8_t value) {    
  analogWrite(pins.LEFT_PWM, value);
  leftSpeed = value;
}

void MotionControl::setRightSpeed(uint8_t value) {
  analogWrite(pins.RIGHT_PWM, value);  
  rightSpeed = value;
}

void MotionControl::setSpeed(uint8_t leftSpeed, uint8_t rightSpeed) {
  setLeftSpeed(leftSpeed);
  setRightSpeed(rightSpeed);
}

void MotionControl::setSpeed(uint8_t value) {
  setSpeed(value, value);  
}

void MotionControl::setSpeedLevel(uint8_t level) {
  if ((level < minSpeedLevel) || (level > maxSpeedLevel)) {
    return;
  }

  double val = ceil(minSpeed + level * levelToSpeedFactor); 
  uint8_t speed = static_cast<uint8_t>(val);
  setSpeed(speed);
}

void MotionControl::incrSpeed() {
  // uint16_t since addition can overflow uint8_t
  uint16_t val = leftSpeed + speedStep;
  val = min(static_cast<uint16_t>(maxSpeed), val);
  uint8_t speed = static_cast<uint8_t>(val);  
  setSpeed(speed);
}

void MotionControl::decrSpeed() {
  //int since subtraction can underflow uint8_t
  int val = leftSpeed - speedStep;
  val = max(static_cast<int>(minSpeed), val);  
  uint8_t speed = static_cast<uint8_t>(val);
  setSpeed(speed, speed);
}

void MotionControl::stop() {
  setSpeed(minSpeed);
}

void MotionControl::setDirection(Side side, Direction dir) {
  // There are only two possible sides
  if ((side != Side::LEFT) && (side != Side::RIGHT)) {    
    return;
  }

  // There are only two possible directions
  if ((dir != Direction::FWD) && (dir != Direction::REV)) {
    return;
  }

  uint8_t in1, in2;
  if (side == Side::LEFT) {
    in1 = pins.LEFT_IN1;
    in2 = pins.LEFT_IN2;
  } else {
    in1 = pins.RIGHT_IN1;
    in2 = pins.RIGHT_IN2;
  }

  if (dir == Direction::FWD) {
    digitalWrite(in1, LOW);
    digitalWrite(in2, HIGH);
  } else {
    digitalWrite(in1, HIGH);
    digitalWrite(in2, LOW);
  }
}

void MotionControl::leftFwd() {
  setDirection(Side::LEFT, Direction::FWD);
}

void MotionControl::leftReverse() {
  setDirection(Side::LEFT, Direction::REV);  
}

void MotionControl::rightFwd() {
  setDirection(Side::RIGHT, Direction::FWD);  
}

void MotionControl::rightReverse() {
  setDirection(Side::RIGHT, Direction::REV);
}

void MotionControl::fwd() {
  leftFwd();
  rightFwd();
}

void MotionControl::reverse() {
  leftReverse();
  rightReverse();
}

void MotionControl::leftIndicatorOn() {
  digitalWrite(pins.LEFT_INDICATOR, HIGH);  
}

void MotionControl::leftIndicatorOff() {
  digitalWrite(pins.LEFT_INDICATOR, LOW);  
}

void MotionControl::rightIndicatorOn() {
  digitalWrite(pins.RIGHT_INDICATOR, HIGH);
}

void MotionControl::rightIndicatorOff() {
  digitalWrite(pins.RIGHT_INDICATOR, LOW);
}  

void MotionControl::turnLeft() {
  leftIndicatorOn();
  leftReverse();
  delay(500);
  leftFwd();  
  leftIndicatorOff();
}

void MotionControl::turnRight() {
  rightIndicatorOn();
  rightReverse();
  delay(500);
  rightFwd();
  rightIndicatorOff();
}

