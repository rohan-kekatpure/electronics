#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "rover.h"
#include "pinout.h"
#include "ircodes.h"

// Rover::Rover(const PINS& pins): pins{pins}, motionControl(pins), irSensor(pins) {}


Rover::Rover(const PINS& pins): pins{pins}, 
                                motionControl(pins), 
                                irSensor(pins) 
{}

void Rover::init() {
  pins.init();
  irSensor.init();
  motionControl.fwd();
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
      Serial.println("BUTTON LEFT");
      motionControl.turnLeft();
      break;
    
    // Right turn
    case IRCODES::LG_BTN_RIGHT_ARROW:
    case IRCODES::ELEGOO_BTN_RIGHT:
      Serial.println("BUTTON RIGHT");
      motionControl.turnRight();
      break;

    // Increase speed
    case IRCODES::LG_BTN_UP_ARROW:
    case IRCODES::ELEGOO_BTN_UP:
      Serial.println("BUTTON UP");
      motionControl.incrSpeed();
      break;
    
    // Backup
    case IRCODES::LG_BTN_DOWN_ARROW:
    case IRCODES::ELEGOO_BTN_DOWN:
      Serial.println("BUTTON DOWN");
      motionControl.backup();      
      break;

    // Stop the car
    case IRCODES::LG_BTN_OK:
    case IRCODES::ELEGOO_BTN_OK:
      Serial.println("BUTTON OK");
      motionControl.stop();
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
      Serial.println("BUTTON 3");
      motionControl.setSpeedLevel(3);
      break;

    // Speed setting 4
    case IRCODES::LG_BTN_4:
    case IRCODES::ELEGOO_BTN_4:
      Serial.println("BUTTON 4");
      motionControl.setSpeedLevel(4);
      break;

    // Speed setting 5
    case IRCODES::LG_BTN_5:
    case IRCODES::ELEGOO_BTN_5:
      Serial.println("BUTTON 5");
      motionControl.setSpeedLevel(5);
      break;

    // Speed setting 6
    case IRCODES::LG_BTN_6:
    case IRCODES::ELEGOO_BTN_6:
      Serial.println("BUTTON 6");
      motionControl.setSpeedLevel(6);
      break;

    // Speed setting 7
    case IRCODES::LG_BTN_7:
    case IRCODES::ELEGOO_BTN_7:
      Serial.println("BUTTON 7");
      motionControl.setSpeedLevel(7);
      break;

    // Speed setting 8
    case IRCODES::LG_BTN_8:
    case IRCODES::ELEGOO_BTN_8:
      Serial.println("BUTTON 8");
      motionControl.setSpeedLevel(8);
      break;

    // Speed setting 9
    case IRCODES::LG_BTN_9:
    case IRCODES::ELEGOO_BTN_9:
      Serial.println("BUTTON 9");
      motionControl.setSpeedLevel(9);
      break;

    // Stop the car
    case IRCODES::LG_BTN_0:
    case IRCODES::ELEGOO_BTN_0:
      Serial.println("BUTTON 0");
      motionControl.decrSpeed();
      break;

    // Stop the car
    case IRCODES::ELEGOO_BTN_STAR:
      Serial.println("BUTTON STAR");      
      break;

    // Stop the car
    case IRCODES::LG_BTN_POWER:
    case IRCODES::ELEGOO_BTN_HASH:
      Serial.println("BUTTON HASH");
      motionControl.stop();
      break;

    default:
      Serial.print("UNKNOWN CODE: ");
      Serial.println(irCode, HEX);
      break;
  }  
  // delay(100);  
}
