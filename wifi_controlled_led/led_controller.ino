#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <Arduino_JSON.h>

const unsigned int MILLIS = 1;
const char* SSID = "REPLACE_WITH_WIFI_NAME";
const char* PASSWD = "REPLACE_WITH_WIFI_PASSWD";
const String LED_URL = "http://192.168.1.67:5000/LED";
const unsigned int REFRESH_DELAY = 500 * MILLIS;

void setup() {
  pinMode(D1, OUTPUT);
  Serial.begin(115200);

  WiFi.begin(SSID, PASSWD);
  Serial.print("\nConnecting to: ");
  Serial.println(SSID);

  unsigned int i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(++i);
  }

  Serial.println('\n');
  Serial.println("Connection established");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  delay(REFRESH_DELAY);
  WiFiClient client;
  HTTPClient http;

  if (WiFi.status() != WL_CONNECTED) {
    return;
  }

  http.begin(client, LED_URL.c_str());
  int responseCode = http.GET();

  if (responseCode < 0) {
    Serial.printf("\n[%d error]: ", responseCode);
    return;
  }

  Serial.printf("\n[%d ok]: ", responseCode);
  String responsePayload = http.getString();
  // Serial.println(responsePayload);
  JSONVar reading = JSON.parse(responsePayload);

  if (JSON.typeof(reading) == "undefined") {
    Serial.println("\nJSON parsing failed");
    return;
  }

  double ledState = double(reading["led_state"]);
  Serial.printf("Reading = %f", ledState);

  // Update LED duty cycle
  // analogWrite(D1, 100.0 * ledState);
  analogWrite(D1, int(255 * ledState));
}
