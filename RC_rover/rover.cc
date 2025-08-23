#include <vector>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "rover.h"
#include "pinout.h"
#include "ircodes.h"
#include "trip.h"

Rover::Rover(const PINS& pins)
  : pins{pins}, motionControl(pins), irSensor(pins) {}

void Rover::init() {
  pins.init();
  irSensor.init();
  motionControl.setModeFwd();  
  addMoveStop();  
}

void Rover::addMoveLeftTurn() {    
  trip.addMove(Move('F', 'L', motionControl.leftSpeed, 0));
}

void Rover::addMoveRightTurn() {    
  trip.addMove(Move('F', 'R', motionControl.leftSpeed, 0));
}

void Rover::addMoveTranslate() {
  trip.addMove(Move('F', 'T', motionControl.leftSpeed, 0));
}

void Rover::addMoveStop() {
  trip.addMove(Move('F', 'S', motionControl.leftSpeed, 0));
}

void Rover::addMoveBackup() {
  trip.addMove(Move('F', 'B', motionControl.leftSpeed, 0));
}

void Rover::setSpeedLevel(uint8_t value) {
  trip.endLastMove();      
  motionControl.setSpeedLevel(value);
  addMoveTranslate();  
}

void Rover::incrSpeed() {
  trip.endLastMove();
  motionControl.incrSpeed();
  addMoveTranslate();
}

void Rover::decrSpeed() {
  trip.endLastMove();
  motionControl.decrSpeed();
  addMoveTranslate();    
}

void Rover::backup() {
  trip.endLastMove();
  motionControl.backup();
  addMoveBackup();
  addMoveStop();
}

void Rover::turnLeft() {
  trip.endLastMove();      
  motionControl.turnLeft();
  addMoveLeftTurn();
  addMoveTranslate();
}

void Rover::turnRight() {
  trip.endLastMove();      
  motionControl.turnRight();
  addMoveRightTurn();
  addMoveTranslate();
}

void Rover::stop() {
  trip.endLastMove();  
  motionControl.stop();
  addMoveStop();
}

void Rover::listen() {  
  uint64_t irCode = irSensor.listen();  
  if (irCode == 0) {
    return;
  }
  switch (irCode) {

    // Left turn
    case IRCODES::LG_BTN_LEFT_ARROW:
    case IRCODES::ELEGOO_BTN_LEFT:
      Serial.println(IRCODES::LG_BTN_LEFT_ARROW, HEX);
      turnLeft();
      break;
    
    // Right turn
    case IRCODES::LG_BTN_RIGHT_ARROW:
    case IRCODES::ELEGOO_BTN_RIGHT:
      Serial.println(IRCODES::LG_BTN_RIGHT_ARROW, HEX);
      turnRight();
      break;

    // Increase speed
    case IRCODES::LG_BTN_UP_ARROW:
    case IRCODES::ELEGOO_BTN_UP:
      Serial.println(IRCODES::LG_BTN_UP_ARROW, HEX);
      incrSpeed();
      break;
    
    // Backup
    case IRCODES::LG_BTN_DOWN_ARROW:
    case IRCODES::ELEGOO_BTN_DOWN:
      Serial.println(IRCODES::LG_BTN_DOWN_ARROW, HEX);    
      backup();
      break;

    // Stop the car
    case IRCODES::LG_BTN_OK:
    case IRCODES::ELEGOO_BTN_OK:
      Serial.println(IRCODES::LG_BTN_RIGHT_ARROW, HEX);
      stop();
      break;

    // Trick 1
    case IRCODES::LG_BTN_1:
    case IRCODES::ELEGOO_BTN_1:
      Serial.println(IRCODES::LG_BTN_1, HEX);      
      motionControl.goCircle(1);
      break;

    // Trick 2
    case IRCODES::LG_BTN_2:
    case IRCODES::ELEGOO_BTN_2:      
      Serial.println(IRCODES::LG_BTN_2, HEX);
      motionControl.goBackAndFwd();
      break;

    // Trick 3
    case IRCODES::LG_BTN_3:
    case IRCODES::ELEGOO_BTN_3:
      Serial.println(IRCODES::LG_BTN_3, HEX);
      if (PINS::ENABLE_CONTS_SNAKE){
        motionControl.goSnake();
      } else {
        motionControl.snake10();
      }      
      break;

    // Speed setting 4
    case IRCODES::LG_BTN_4:
    case IRCODES::ELEGOO_BTN_4:
      Serial.println(IRCODES::LG_BTN_4, HEX);
      setSpeedLevel(4);
      break;

    // Speed setting 5
    case IRCODES::LG_BTN_5:
    case IRCODES::ELEGOO_BTN_5:
      Serial.println(IRCODES::LG_BTN_5, HEX);
      setSpeedLevel(5);
      break;

    // Speed setting 6
    case IRCODES::LG_BTN_6:
    case IRCODES::ELEGOO_BTN_6:
      Serial.println(IRCODES::LG_BTN_6, HEX);
      setSpeedLevel(6);
      break;

    // Speed setting 7
    case IRCODES::LG_BTN_7:
    case IRCODES::ELEGOO_BTN_7:
      Serial.println(IRCODES::LG_BTN_7, HEX);
      setSpeedLevel(7);
      break;

    // Speed setting 8
    case IRCODES::LG_BTN_8:
    case IRCODES::ELEGOO_BTN_8:
      Serial.println(IRCODES::LG_BTN_8, HEX);
      setSpeedLevel(8);
      break;

    // Speed setting 9
    case IRCODES::LG_BTN_9:
    case IRCODES::ELEGOO_BTN_9:
      Serial.println(IRCODES::LG_BTN_9, HEX);
      setSpeedLevel(9);
      break;

    // Stop the car
    case IRCODES::LG_BTN_0:
    case IRCODES::ELEGOO_BTN_0:
      Serial.println(IRCODES::LG_BTN_0, HEX);
      decrSpeed();
      break;

    // Stop the car
    case IRCODES::LG_BTN_UNDO:
    case IRCODES::LG_BTN_FLASHBK:
    case IRCODES::ELEGOO_BTN_STAR:      
      Serial.println(IRCODES::LG_BTN_UNDO, HEX);
      Serial.println("Trip:");  
      Serial.println(trip.toString().c_str());
      Serial.println("Reverse trip:");
      Serial.println(trip.reverse().toString().c_str());
      motionControl.setModeRev();
      motionControl.execTrip(trip.reverse());
      trip.clear();    
      motionControl.setModeFwd();
      break;

    // Stop the car
    case IRCODES::LG_BTN_POWER:
    case IRCODES::ELEGOO_BTN_HASH:
      Serial.println(IRCODES::LG_BTN_POWER, HEX);
      stop();
      break;

    default:
      // Serial.print("UNKNOWN CODE: ");
      // Serial.println(irCode, HEX);
      break;
  }  
  // delay(100);  
}
