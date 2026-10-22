/*
Watch https://www.youtube.com/watch?v=sycSdI49hlY
for instructions on how to program ATTINY85 using
Arduino UNO as an ISP
*/

#include<TinyWireM.h>
#include <Tiny4kOLED.h>

uint8_t width = 128;
uint8_t height = 64;

void setup() {
  oled.begin(width, height, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
  oled.setFont(FONT8X16);  
  hatch();
  drawScreen();
  oled.on();
}

void loop() {
  scrollScreen();
  drawScreen();
}

void hatch() {
  // Set entire memory to hatched - if you see any of this hatching, then the display is not initialised correctly.
  for (uint8_t y = 0; y < 8; y++) {
    oled.setCursor(0, y);
    oled.startData();
    for (uint8_t x=0; x<128; x += 2) {
      oled.sendData(0b10101010);
      oled.sendData(0b01010101);
    }
    oled.endData();
  }
}

void drawScreen() {
  oled.setCursor(0, 0);
  oled.startData();
  oled.sendData(0b11111111);
  oled.repeatData(0b00000001, width - 2);
  oled.sendData(0b11111111);
  oled.endData();

  for (uint8_t y = 1; y < (height - 8) / 8; y++) {
    oled.setCursor(0, y);
    oled.startData();
    oled.sendData(0b11111111);
    oled.repeatData(0b00000000, width - 2);
    oled.sendData(0b11111111);
    oled.endData();
  }

  oled.setCursor(0, (height - 8) / 8);
  oled.startData();
  oled.sendData(0b11111111);
  oled.repeatData(0b10000000, width - 2);
  oled.sendData(0b11111111);
  oled.endData();

  oled.setCursor(8, 1);
  oled.print(width);
  oled.print('x');  
  oled.print(height);
}

void scrollScreen() {
  uint8_t startScrollPage = 1;
  uint8_t endScrollPage = 2;
  uint8_t startScrollColumn = 8;
  uint8_t endScrollColumn = startScrollColumn + width - 16;
  for (uint8_t x = 0; x < width - 16; x++)
  {
    delay(20);
    oled.scrollContentRight(startScrollPage, endScrollPage, startScrollColumn, endScrollColumn);
  }
}