/*
Example of a ESP8266-based webserver where we serve the 
static file directly from the LittleFS filesystem.
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>

const char* SSID = "SSID";
const char* PASSWD = "PASSWD";

ESP8266WebServer server(80);

void serveIndex() {
  if (! LittleFS.begin()) {
    Serial.println("Error mounting LittleFS");    
  }
  server.serveStatic("/", LittleFS, "/index.html");  
}

void setup() {    
  Serial.begin(115200);

  Serial.printf("Connecting to: %s\n", SSID);
  WiFi.begin(SSID, PASSWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Connected");
  Serial.println(WiFi.localIP()); 

  serveIndex();
  server.begin();
}

void loop() {
  server.handleClient();
}
