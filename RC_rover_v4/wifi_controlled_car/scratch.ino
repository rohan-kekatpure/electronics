#include "LittleFS.h"

void _setup() {
  Serial.begin(115200);
  if (! LittleFS.begin()) {
    Serial.println("Error mounting LittleFS");
    return;
  }

  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    Serial.println("Failed to open file");
    return;
  }

  Serial.println("File contents");
  while (file.available()) {
    Serial.write(file.read());
  }

  file.close();

}

void _loop() {}