#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>

#include "rover.h"
#include "pinout.h"

// Rover
const PINS& pinout = PINS();
Rover rover(pinout);

void setup() {     
  Serial.begin(115200);  
  rover.init();    
}

void loop() {
  // server.handleClient();
  rover.listen();
}
