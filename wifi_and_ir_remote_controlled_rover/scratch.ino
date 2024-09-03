// #include <Arduino.h>
// #include <IRremoteESP8266.h>
// #include <IRrecv.h>
// #include <IRutils.h>

// const uint16_t kRecvPin = D2;
// IRrecv irrecv(kRecvPin);
// decode_results results;

// void setup() {  
//   Serial.begin(115200);
//   irrecv.enableIRIn();  // Start the receiver
//   Serial.println();
//   Serial.print("WFITMOP ");
//   Serial.println(kRecvPin);
// }

// void loop() {
//   if (irrecv.decode(&results)) {
//     serialPrintUint64(results.value, HEX);
//     Serial.println("");
//     irrecv.resume();  // Receive the next value
//   }
//   delay(100);
// }