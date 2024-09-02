#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <LittleFS.h>
#include <Arduino_JSON.h>


const char* SSID = "SSID";
const char* PASSWD = "PASSWD";
const int PWM = 255;
double GLOBAL_LEFT_SPEED = 100.0;
double GLOBAL_RIGHT_SPEED = 100.0;
const uint8_t LEFT_SIDE_CONTROL_PIN = D1;
const uint8_t RIGHT_SIDE_CONTROL_PIN = D2;

ESP8266WebServer server(80);

void serveIndex() {
  if (! LittleFS.begin()) {
    Serial.println("Error mounting LittleFS");    
  }
  server.serveStatic("/", LittleFS, "/index.html");  
}

void handlexy() {
  JSONVar coords = JSON.parse(server.arg(0));
  
  if (JSON.typeof(coords) == "undefined"){
    Serial.println(server.arg(0));
    Serial.println("Failed to parse request payload");
    return;
  }

  // Received from the front end
  double x = coords["x"];
  double y = coords["y"];

  // Derived parameters
  double r = sqrt(x * x + y * y);
  double theta_rad = PI / 2.0 - atan2(y, x);
  double theta_deg = theta_rad * RAD_TO_DEG;

  // Conversion to motor PWMs
  double base_speed = r; // speed is distance from the center
  double left_speed = base_speed;
  double right_speed = base_speed;
  double factor = 1.0 - fabs(sin(theta_rad));

  if ((theta_deg >= 0.0) && (theta_deg <= 90.0)) {
    right_speed *= factor;    
  } else if ((theta_deg < 0.0) && (theta_deg >= -90.0)) {
    left_speed *= factor;
  }

  // Convert speeds to PWM
  left_speed *= PWM;
  right_speed *= PWM; 

  // Change speed state
  GLOBAL_LEFT_SPEED = left_speed;
  GLOBAL_RIGHT_SPEED = right_speed;  

  String fmt = "LEFT_SPEED = %0.2f, RIGHT_SPEED = %0.2f\n";
  Serial.printf(fmt.c_str(), GLOBAL_LEFT_SPEED, GLOBAL_RIGHT_SPEED);

  // Write to Drive pins
  analogWrite(LEFT_SIDE_CONTROL_PIN, GLOBAL_LEFT_SPEED);
  analogWrite(RIGHT_SIDE_CONTROL_PIN, GLOBAL_RIGHT_SPEED);
}

void setup() {    
  // Set up Pins
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);

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
  server.on("/xy", handlexy);
  server.begin();
  
}

void loop() {
  server.handleClient();
}
