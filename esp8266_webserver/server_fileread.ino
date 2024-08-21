/*
Example of a ESP8266-based webserver where we serve the 
static file from the LittleFS filesystem by reading in 
the file into a string.
*/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>

const char* SSID = "WIFI_NAME";
const char* PASSWD = "WIFI_PASSWD";

ESP8266WebServer server(80);

String readIndexHtml() {
  if (! LittleFS.begin()) {
    Serial.println("Error mounting LittleFS");
    return "";
  }

  /*
  The `index.html` is loaded onto the flash memory of 
  ESP8266 using the LittleFS filesystem uploader installed
  on to the Arduino IDE. Google for how to install the 
  LittleFS filesystem uploader and how to access the uploaded
  files.
  */
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    Serial.println("Failed to open file");
    return "";
  }

  String html = "";
  char c;

  while (file.available()) {
    c = file.read();    
    html += c;    
  }

  file.close();  
  return html;

}

void handleRoot() {  
  const String html = readIndexHtml();  
  server.send(200, "text/html", html);
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

  // Register root handler
  server.on("/", handleRoot);  

  // Start the server
  server.begin();

}

void loop() {
  server.handleClient();
}
