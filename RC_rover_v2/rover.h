#ifndef _ROVER_H
#define _ROVER_H

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "motioncontrol.h"
#include "irsensor.h"
#include "pinout.h"

class Rover {
  const PINS& pins;  
  MotionControl motionControl;
  IRSensor irSensor;

  public:
  Rover(const PINS& pins);
  void init();
  void listen();
};
#endif