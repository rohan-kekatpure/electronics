#include <Arduino.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>

// NodeMCU GPIO pin to use
const uint16_t kIrLed = D2;

// Initialize the GPIO pin
IRsend irsend(kIrLed);  

void setup() {
  irsend.begin();

#if ESP8266
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);
#else  // ESP8266
  Serial.begin(115200, SERIAL_8N1);
#endif  // ESP8266
}

void loop() {
  Serial.println("NEC");
  uint64_t ON_OFF = 0x20DF10EF;
  irsend.sendNEC(ON_OFF, 32);
  delay(2000);
}