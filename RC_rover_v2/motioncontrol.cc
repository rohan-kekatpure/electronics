#include<Arduino.h>
#include "pinout.h"
#include "motioncontrol.h"

MotionControl::MotionControl(const PINS& pins): pins{pins} {} 

void MotionControl::init() {
  leftSpeed = 0;
  rightSpeed = 0;
  leftSpeedLevel = 0;
  rightSpeedLevel = 0;
}

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

  uint8_t speed = map(level, 0, 9, 0, 255);
  setSpeed(speed);
  leftSpeedLevel = level;
  rightSpeedLevel = level;
}

void MotionControl::incrSpeed() {
  /*
    Currently usable only if leftSpeedLevel == rightSpeedLevel
    will change to a more interesting function later which will 
    allow increase of left and right levels even when they are
    unequal
  */
  if (leftSpeedLevel != rightSpeedLevel) {
    return;
  }  
  uint8_t currentSpeedLevel = leftSpeedLevel;
  setSpeedLevel(++currentSpeedLevel);
}

void MotionControl::decrSpeed() {
  if (leftSpeedLevel != rightSpeedLevel) {
    return;
  }  
  uint8_t currentSpeedLevel = leftSpeedLevel;
  setSpeedLevel(--currentSpeedLevel);
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

void MotionControl::backup() {
  // function for a short distance backup
  reverse();
  setSpeedLevel(6);
  delay(500);  
  stop();
  fwd();
}

void MotionControl::turnMoveHalt(Turn dir) {
  fwd();
  setSpeedLevel(6);
  dir == Turn::LEFT ? turnLeft() : turnRight();
  delay(500);
  stop();
}
    
void MotionControl::goCircle(uint8_t size) {
  fwd();
  uint8_t rightSpeed = map(size, 1, 9, 70, 255);
  setRightSpeed(rightSpeed);
  setLeftSpeed(255);
}

void MotionControl::goSnake() {
  fwd();
  setSpeedLevel(6);
  while (true) {
    turnMoveHalt(Turn::LEFT);
    delay(100);
    turnMoveHalt(Turn::RIGHT);
    delay(100);
  }
}

void MotionControl::goRandomDirection() {}
void MotionControl::goSpiral() {}
void MotionControl::goSquare() {}

