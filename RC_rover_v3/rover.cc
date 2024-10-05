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
      turnLeft();
      break;
    
    // Right turn
    case IRCODES::LG_BTN_RIGHT_ARROW:
    case IRCODES::ELEGOO_BTN_RIGHT:
      turnRight();
      break;

    // Increase speed
    case IRCODES::LG_BTN_UP_ARROW:
    case IRCODES::ELEGOO_BTN_UP:
      incrSpeed();
      break;
    
    // Backup
    case IRCODES::LG_BTN_DOWN_ARROW:
    case IRCODES::ELEGOO_BTN_DOWN:    
      backup();
      break;

    // Stop the car
    case IRCODES::LG_BTN_OK:
    case IRCODES::ELEGOO_BTN_OK:
      stop();
      break;

    // Trick 1
    case IRCODES::LG_BTN_1:
    case IRCODES::ELEGOO_BTN_1:
      Serial.println("BUTTON 1");
      motionControl.goCircle(1);
      break;

    // Trick 2
    case IRCODES::LG_BTN_2:
    case IRCODES::ELEGOO_BTN_2:
      Serial.println("BUTTON 2");
      motionControl.goCircle(3);      
      break;

    // Speed setting 3
    case IRCODES::LG_BTN_3:
    case IRCODES::ELEGOO_BTN_3:
      setSpeedLevel(3);
      break;

    // Speed setting 4
    case IRCODES::LG_BTN_4:
    case IRCODES::ELEGOO_BTN_4:
      setSpeedLevel(4);
      break;

    // Speed setting 5
    case IRCODES::LG_BTN_5:
    case IRCODES::ELEGOO_BTN_5:
      setSpeedLevel(5);
      break;

    // Speed setting 6
    case IRCODES::LG_BTN_6:
    case IRCODES::ELEGOO_BTN_6:
      setSpeedLevel(6);
      break;

    // Speed setting 7
    case IRCODES::LG_BTN_7:
    case IRCODES::ELEGOO_BTN_7:
      setSpeedLevel(7);
      break;

    // Speed setting 8
    case IRCODES::LG_BTN_8:
    case IRCODES::ELEGOO_BTN_8:
      setSpeedLevel(8);
      break;

    // Speed setting 9
    case IRCODES::LG_BTN_9:
    case IRCODES::ELEGOO_BTN_9:
      setSpeedLevel(9);
      break;

    // Stop the car
    case IRCODES::LG_BTN_0:
    case IRCODES::ELEGOO_BTN_0:
      decrSpeed();
      break;

    // Stop the car
    case IRCODES::LG_BTN_UNDO:
    case IRCODES::LG_BTN_FLASHBK:
    case IRCODES::ELEGOO_BTN_STAR:      
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
      stop();
      break;

    default:
      // Serial.print("UNKNOWN CODE: ");
      // Serial.println(irCode, HEX);
      break;
  }  
  // delay(100);  
}
