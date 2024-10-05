#include<Arduino.h>
#include "pinout.h"
#include "motioncontrol.h"

MotionControl::MotionControl(const PINS& pins)
  : pins{pins}, leftSpeed{0}, rightSpeed{0},
  rightSpeedLevel{0}, leftSpeedLevel{0} {} 

void MotionControl::setModeFwd() {
  mode = Direction::FWD;
  pinConfigFwd();
}

void MotionControl::setModeRev() {
  mode = Direction::REV;
  pinConfigRev();
}

void MotionControl::toggleMode() {
  mode == Direction::FWD? setModeRev() : setModeFwd();
}

/*
  Utility function to restore the motor at current speed. This function
  is needed after direction change.
*/
void MotionControl::disable() {
  digitalWrite(LPWM, LOW);
  digitalWrite(RPWM, LOW);
}

/*
  Utility function to restore the motor at current speed. This function
  is needed after direction change.
*/
void MotionControl::restore() {
  setSpeed(this->leftSpeed, this->rightSpeed);
}

/*
  Configures pins for forward mode motion
*/
void MotionControl::pinConfigFwd() {    
    LPWM = pins.LEFT_PWM;
    RPWM = pins.RIGHT_PWM;
    LIN1 = pins.LEFT_IN1;
    LIN2 = pins.LEFT_IN2;
    RIN1 = pins.RIGHT_IN1;
    RIN2 = pins.RIGHT_IN2;
}

/*
  Pin configuration for reverse mode motion. Reverse mode motion 
  flips left <> right as well as IN1 and IN2 pins for each side.
*/
void MotionControl::pinConfigRev() {  
    LPWM = pins.RIGHT_PWM;
    RPWM = pins.LEFT_PWM;
    LIN1 = pins.RIGHT_IN2;
    LIN2 = pins.RIGHT_IN1;
    RIN1 = pins.LEFT_IN2;
    RIN2 = pins.LEFT_IN1;
}

void MotionControl::leftFwd() {
  disable();
  digitalWrite(LIN1, LOW);
  digitalWrite(LIN2, HIGH);  
  restore();
}

void MotionControl::leftReverse() {
  disable();
  digitalWrite(LIN1, HIGH);
  digitalWrite(LIN2, LOW);
  restore();
}

void MotionControl::rightFwd() {
  disable();
  digitalWrite(RIN1, LOW);
  digitalWrite(RIN2, HIGH);  
  restore();
}

void MotionControl::rightReverse() {
  disable();
  digitalWrite(RIN1, HIGH);
  digitalWrite(RIN2, LOW);  
  restore();
}

void MotionControl::fwd() {
  leftFwd();
  rightFwd();  
}

void MotionControl::reverse() {
  leftReverse();
  rightReverse();  
}

void MotionControl::setLeftSpeed(uint8_t value) {    
  analogWrite(LPWM, value);
  leftSpeed = value;
}

void MotionControl::setRightSpeed(uint8_t value) {
  analogWrite(RPWM, value);  
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

void MotionControl::execMove(const Move &move) {
  // Set direction
  move.dir == 'F'? fwd() : reverse();

  // Detect motion type
  switch (move.moveType) {
    case 'L':
      turnLeft();
      break;
    case 'R':
      turnRight();
      break;
    case 'B':
      backup();
      break;
    case 'T':
      setSpeed(move.speed);
      delay(move.duration);      
      break;
    case 'S':
      stop();
      break;
    default:
      break;
  }  
}

void MotionControl::execTrip(const Trip &trip) {
  for (const Move& move: trip.getMoves()) {
    execMove(move);
  }
}

void MotionControl::goRandomDirection() {}
void MotionControl::goSpiral() {}
void MotionControl::goSquare() {}

