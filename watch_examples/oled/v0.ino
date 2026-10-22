#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdio.h> 

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned const _1_SECOND = 1000;
unsigned const _1_MINUTE = 60;
unsigned const _1_HOUR = 60;
unsigned const _1_DAY = 24;

unsigned DAY = 26, MONTH = 8, YEAR = 2025;
unsigned HH = 5, MM = 3, _SS = 0;
unsigned long NOW, STARTTIME;

void getTime(unsigned const hh, unsigned const mm, unsigned const ss, char* timestr) {
  if (timestr == NULL) {
    return;
  }
  snprintf(timestr, 9, "%.2u:%.2u:%.2u", hh, mm, ss);
}

void getDate(unsigned day, unsigned month, unsigned year, char* datestr) {
  if (datestr == NULL){
    return;
  }
  snprintf(datestr, 11, "%.2u/%.2u/%.4u", month, day, year);
}

void setup() {  
  Serial.begin(115200);
  delay(1000);    
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }      
  display.clearDisplay();
  display.display();
  display.setTextColor(SSD1306_WHITE);

  STARTTIME = millis();
}

void loop() {
  char datebuf[11];
  char timebuf[9];
  
  NOW = millis();
  if ((NOW - STARTTIME) >= _1_SECOND){
    _SS++;
    STARTTIME = NOW;
  }

  if (_SS >= _1_MINUTE) {
    _SS = 0;
    MM++;
  }

  if (MM >= _1_HOUR) {
    MM = 0;
    HH++;
  }

  if (HH >= _1_DAY) {
    HH = 0;
    DAY++;
  }
  
  getDate(DAY, MONTH, YEAR, datebuf);
  getTime(HH, MM, _SS, timebuf);

  // Serial.println(datebuf);
  // Serial.println(timebuf);
  // Serial.println();

  display.clearDisplay();
  display.setCursor(0,  5);

  display.setTextSize(2, 2);         
  display.println(datebuf);

  display.setTextSize(1);  
  display.println();  

  display.setTextSize(2, 4);  
  display.println(timebuf);

  display.display();
  // delay(10);  
}