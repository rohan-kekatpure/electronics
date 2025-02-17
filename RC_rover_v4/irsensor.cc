#include "irsensor.h"
#include "pinout.h"

IRSensor::IRSensor(const PINS& pins): irrecv(pins.IRRECV, 1024, 15, true) {}

void IRSensor::init() {
  irrecv.enableIRIn();  
}

uint64_t IRSensor::listen() {    
  if (irrecv.decode(&result)) {     
    irrecv.resume();
    return result.value;
  }       
  yield();  
  return 0;
}


