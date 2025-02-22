#include <Arduino.h>
#include <assert.h>
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRac.h>
#include <IRtext.h>
#include <IRutils.h>

uint8 IRPIN = D5;
uint8 IR_INTERRUPT_PIN = D7;
IRrecv irrecv(IRPIN, 1024, 50, true);
decode_results results;  
volatile bool loopFlag;

void setup() {
  Serial.begin(115200);     
  pinMode(IR_INTERRUPT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(IR_INTERRUPT_PIN), myISR, RISING);
  irrecv.enableIRIn(); 
}

// The repeating section of the code
void loop() {  
  loopFlag = true;
  uint64_t val = listen();
  if (val > 0) {
    Serial.println(val, HEX);    
  }
  switch(results.value) {
    case 0x20DF10EF: //power btn
      looptask();
      break;
    default:
      break;
  }
}

void looptask() {
  while(loopFlag) {
    Serial.println("in while loop");    
    delay(1000);
  }
}

IRAM_ATTR void myISR() {
  loopFlag = false;
}

uint64_t listen() {      
  if (irrecv.decode(&results)) {     
    irrecv.resume();    
    return results.value;    
  }             
  yield();
  return 0;
}
