#ifndef _IRSENSOR_H
#define _IRSENSOR_H

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

#include "pinout.h"

class IRSensor {    
  decode_results result;
  IRrecv irrecv;

  public:
  IRSensor(const PINS& p);
  void init();
  uint64_t listen();  
};

#endif