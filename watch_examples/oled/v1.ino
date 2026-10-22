#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <stdio.h>
#include <FspTimer.h>

/* Watch related constants */
uint64_t count = 0;
unsigned short seconds = 0;
unsigned short minute = 17;
unsigned short hour = 16;
unsigned short day = 29;
unsigned short month = 8;
unsigned short year = 2025;
unsigned short ticksPerSecond = 4;
FspTimer timer;

/* Display related constants */
const unsigned short SCREEN_WIDTH = 128; // OLED display width, in pixels
const unsigned short SCREEN_HEIGHT = 64; // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
const unsigned short OLED_RESET = -1; // Reset pin # (or -1 if sharing Arduino reset pin)
const unsigned short SCREEN_ADDRESS = 0x3C;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Interrupt service routine
void timer_callback(timer_callback_args_t __attribute((unused)) *p_args) {
  count++;
}

void updateDateAndTime() {
  if (count == ticksPerSecond) {
    seconds++;
    count = 0;
  }

  if (seconds == 60) {
    seconds = 0;
    minute++;    
  }

  if (minute == 60) {
    hour++;
    minute = 0;
  }

  if (hour == 24) {
    day++;
    hour = 0;
  }  

  uint8_t daysInMonth;  
  bool isLeapYear = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);   
  switch (month) {
    case 4: case 6: case 9: case 11:
      daysInMonth = 30;
      break;
    case 2:
      daysInMonth = isLeapYear ? 29 : 28;
      break;
    default:
      daysInMonth = 31;
  }

  if (day > daysInMonth) {
    month++;
    day = 1;    
  }

  if (month > 12) {
    year++;
    month = 1;
  }
}

void displayDateAndTime() {    
  char datebuf[11];
  char timebuf[9];
  fmtTime(timebuf);
  fmtDate(datebuf);
  display.clearDisplay();
  display.setCursor(0,  5);

  display.setTextSize(2, 2);         
  display.println(datebuf);

  display.setTextSize(1);  
  display.println();  

  display.setTextSize(2, 4);  
  display.println(timebuf);

  display.display();
}

bool beginTimer(float rate) {
  uint8_t timer_type = GPT_TIMER;
  int8_t tindex = FspTimer::get_available_timer(timer_type);
  if (tindex < 0){
    tindex = FspTimer::get_available_timer(timer_type, true);
  }
  if (tindex < 0){
    return false;
  }

  FspTimer::force_use_of_pwm_reserved_timer();

  if(!timer.begin(
        TIMER_MODE_PERIODIC, 
        timer_type, 
        tindex, 
        ticksPerSecond, 
        0.0f, 
        timer_callback
      )
    ) {
    return false;
  }

  if (!timer.setup_overflow_irq()){
    return false;
  }

  if (!timer.open()){
    return false;
  }

  if (!timer.start()){
    return false;
  }
  return true;
}

void fmtTime(char* timestr) {
  if (timestr == NULL) {
    return;
  }
  snprintf(timestr, 9, "%.2hu:%.2hu:%.2hu", hour, minute, seconds);
}

void fmtDate(char* datestr) {
  if (datestr == NULL){
    return;
  }
  snprintf(datestr, 11, "%.2hu/%.2hu/%.4hu", month, day, year);
}

void setup() {
  Serial.begin(115200);
  beginTimer(1000);
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }      

  display.clearDisplay();
  display.display();
  display.setTextColor(SSD1306_WHITE);  
}

void loop() {    
  updateDateAndTime();  
  displayDateAndTime();
}