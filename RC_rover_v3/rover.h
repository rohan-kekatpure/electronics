#ifndef _ROVER_H
#define _ROVER_H

#include <vector>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "motioncontrol.h"
#include "irsensor.h"
#include "pinout.h"
#include "trip.h"

class Rover {
  const PINS& pins;  
  MotionControl motionControl;
  IRSensor irSensor;  
  Trip trip;

  public:
  Rover(const PINS& pins);
  void init();
  void listen();

  void addMoveLeftTurn();
  void addMoveRightTurn();
  void addMoveSpeed();  
  void addMoveStop();
  void addMoveBackup();

  void setSpeedLevel(uint8_t value);
  void incrSpeed();
  void decrSpeed();
  void backup();
  void turnLeft();
  void turnRight();
  void stop();

};
#endif