// #include <Arduino.h>
// #include <assert.h>
// #include <IRrecv.h>
// #include <IRremoteESP8266.h>
// #include <IRac.h>
// #include <IRtext.h>
// #include <IRutils.h>

// uint8 IRPIN = D5;
// IRrecv irrecv(IRPIN, 1024, 50, true);
// decode_results results;  // Somewhere to store the results

// void setup() {
//   Serial.begin(115200);     
//   irrecv.enableIRIn();  // Start the receiver
// }

// // The repeating section of the code
// void loop() {  
//   if (irrecv.decode(&results)) {    
//     Serial.print(resultToHumanReadableBasic(&results));    
//     Serial.println();    
//     yield();             

//     switch(results.value) {
//       case 0x20DF10EF: //power btn
//         looptask();
//         break;
//       default:
//         break;
//     }
//   }
// }

// void looptask() {
//   while(true) {
//     Serial.println("in while loop");    
//     delay(1000);
//   }
// }