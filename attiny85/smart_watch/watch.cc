#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <TinyWireM.h>
#include <Tiny4kOLED.h>

// Interrupt flags
volatile uint8_t TICKFLAG = 0;
volatile uint8_t SELECTOR_PRESS = 0;
volatile uint8_t SETTER_PRESS = 0;
volatile uint8_t SELECTOR_DEBOUNCE = 0;
volatile uint8_t SETTER_DEBOUNCE = 0;

uint8_t SELECTOR = 8;
const uint8_t DISPLAY_TIMEOUT = 30;  // Seconds before display turns off

// Date and time structure
struct DateTime {
  uint8_t year;   // Last two digits only
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec;
  uint8_t weekday; // An int from 0 to 6;

  uint8_t daysInMonth() {
    switch (month) {
      case 4: case 6: case 9: case 11:
        return 30;
      case 2:
        return (year & 0x03) ? 28 : 29;  // Fast divisibility by 4 check
      default:
        return 31;
    }
  }

  void tick() {
    // Increment seconds and cascade through time units
    if (++sec == 60) {
      sec = 0;
      min++;
    }

    if (min == 60) {
      hour++;
      min = 0;
    }

    if (hour == 24) {
      day++;      
      hour = 0;      
      weekday++;
    }

    if (weekday > 6) {
      weekday = 0;
    }

    if (day > daysInMonth()) {
      month++;
      day = 1;
    }

    if (month > 12) {
      year++;
      month = 1;
    }
  }
};

DateTime DATETIME = {25, 11, 23, 19, 49, 0};

// Display state with auto-timeout
struct TimedDisplayState {
  bool isOn;
  uint8_t timer;

  void tick() {
    if (timer > 0) {
      timer--;
      if (timer == 0) {
        isOn = false;
      }
    }
  }

  void on() {
    isOn = true;
    timer = DISPLAY_TIMEOUT;
  }
};

volatile TimedDisplayState DISPLAY_STATE = {false, 0};

// Timer interrupt - fires every 8ms, 125 times = 1 second
ISR(TIMER0_COMPA_vect) {
  static uint8_t COUNT = 0;
  COUNT++;
  if (COUNT == 125) {
    COUNT = 0;
    TICKFLAG = 1;  // Signal 1 second elapsed
  }

  // Decrement debounce counters
  if (SELECTOR_DEBOUNCE > 0) {
    SELECTOR_DEBOUNCE--;
  }

  if (SETTER_DEBOUNCE > 0) {
    SETTER_DEBOUNCE--;
  }
}

// Pin change interrupt - handles all three buttons
ISR(PCINT0_vect) {
  // PB3 - selector button
  if ((!(PINB & _BV(PB3))) && (SELECTOR_DEBOUNCE == 0)) {
    SELECTOR_PRESS = 1;
    SELECTOR_DEBOUNCE = 50;  // ~400ms debounce
  }

  // PB4 - setter button
  if ((!(PINB & _BV(PB4))) && (SETTER_DEBOUNCE == 0)) {
    SETTER_PRESS = 1;
    SETTER_DEBOUNCE = 50;
  }

  // PB1 - wake button
  if (!(PINB & _BV(PB1))) {
    DISPLAY_STATE.on();  // Wake display
  }
}

void updateSelector() {
  SELECTOR = (SELECTOR + 1) & 0x0F;  // Modulo 8 using bitwise AND
  SELECTOR_PRESS = 0;
  selectField();
}

void selectField() {
  uint8_t cx, cy;
  uint8_t flen = 12;
  static uint8_t prevSelector = 8;

  uint8_t Y2 = 29;
  // Determine cursor position for each field
  switch (SELECTOR) {
    case 0: cx = 9;  cy = 10; flen = 12; break;  // Month
    case 1: cx = 26; cy = 10; flen = 12; break;  // Day
    case 2: cx = 44; cy = 10; flen = 24; break;  // Year
    case 3: cx = 9;  cy = Y2; flen = 16; break;  // Hour
    case 4: cx = 32; cy = Y2; flen = 16; break;  // Minute
    case 5: cx = 55; cy = Y2; flen = 16; break;  // Second
    case 6: cx = 9; cy = 31; flen = 18; break; // weekday
    case 7: cx = 60; cy = 31; flen = 18; break; // OSCCAL value
    case 8: case 9: case 10: case 11: case 12: case 13: case 14: case 15:
    default:
      break;  // No selection
  }

  if (SELECTOR != prevSelector) {
    // Clear previous highlight
    oled.setCursor(9, 10);
    oled.setFont(FONT6X8);
    oled.clearToEOL();
    
    oled.setCursor(9, Y2);
    oled.setFont(FONT6X8);
    oled.clearToEOL();

    oled.setCursor(9, 31);
    oled.setFont(FONT6X8);
    oled.clearToEOL();

    prevSelector = SELECTOR;

    // Draw new highlight
    if (SELECTOR < 8) {
      oled.setCursor(cx, cy);
      oled.fillLength(0x0F, flen);
    }
  }
}

void setDateTime() {
  SETTER_PRESS = 0;
  DateTime *p = &DATETIME;

  // Increment selected field and wrap around
  switch (SELECTOR) {
    case 0:
      if (++p->month > 12) {
        p->month = 1;
      }
      break;

    case 1:
      if (++p->day > p->daysInMonth()) {
        p->day = 1;
      }
      break;

    case 2:
      if (++p->year > 99) {
        p->year = 25;
      }
      break;

    case 3:
      if (++p->hour > 23) {
        p->hour = 0;
      }
      break;

    case 4:
      if (++p->min > 59) {
        p->min = 0;
      }
      break;

    case 5:
      if (++p->sec > 59) {
        p->sec = 0;
      }
      break;

    case 6:
      if (++p->weekday > 6) {
        p->weekday = 0;
      }      
    case 7:      
      if (++OSCCAL == 0XFF) {
        OSCCAL = 0;
      }
    default:
      break;
  }
}

void displayOn() {  
  oled.on();
}

void displayOff() {  
  oled.off(); 
  SELECTOR = 6;
  updateSelector();
}

void updateDisplay() {  
  DateTime *p = &DATETIME;
  char datebuf[11];
  char timebuf[9];

  // Format date and time strings
  snprintf(datebuf, sizeof(datebuf), "%02d.%02d.20%02d",
           p->month, p->day, p->year);
  snprintf(timebuf, sizeof(timebuf), "%02d:%02d:%02d",
           p->hour, p->min, p->sec);

  char *weekdays[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};

  // Display date
  oled.setCursor(8, 1);
  oled.setFont(FONT6X8);
  oled.print(datebuf);

  // Display time
  oled.setCursor(8, 11);
  oled.setFont(FONT8X16);
  oled.print(timebuf);

  // Display Weekday
  oled.setFont(FONT6X8);
  oled.setCursor(8, 30);    
  oled.print(weekdays[p->weekday]);

  // Show calibration value
  oled.setCursor(36, 30);
  oled.setFont(FONT6X8);

  oled.print("CLK:");
  if (OSCCAL < 10) {
    oled.print("00");
  } else if ((OSCCAL >= 10) && (OSCCAL <= 99)) {
    oled.print("0");
  }  
  oled.print(OSCCAL);  

  // Extra graphics 
  graphic();
}

void drawPixel(int8_t x, int8_t y) {
  /*
    Primitive for drawing a single pixel; will
    be used by all other graphics functions 
  */
  if (x < 0 || x >= 128 || y < 0 || y >= 64) {
    return;
  };
  
  uint8_t page = y >> 3;  // Convert pixel Y to page
  uint8_t bit = y & 0x07;  // Bit position within page
  
  oled.setCursor(x, page);
  oled.startData();   
  oled.sendData(1 << bit);  
  oled.endData();  
}

void drawLine(int8_t x0, int8_t y0, int8_t x1, int8_t y1) {
  /*
    Bresenham's Line Algorithm ; uses only integer arithmetic.
  */
  int8_t dx = x1 - x0;
  int8_t dy = y1 - y0;
  
  if (dx < 0) {
    dx = -dx;
  }
  if (dy < 0) {
    dy = -dy;
  }

  int8_t sx = (x0 < x1) ? 1 : -1;
  int8_t sy = (y0 < y1) ? 1 : -1;
  
  int8_t err = dx - dy;
  
  while (1) {
    drawPixel(x0, y0);
    
    if (x0 == x1 && y0 == y1) break;
    
    int8_t e2 = err << 1;
    
    if (e2 > -dy) {
      err -= dy;
      x0 += sx;
    }
    
    if (e2 < dx) {
      err += dx;
      y0 += sy;
    }
  }
}

// Draw a diamond outline (45-degree rotated square)
void drawDiamond(int cx, int cy, int size) {
  int half = size >> 1;  
  
  // Draw four edges of diamond
  for (int i = 0; i <= half; i++) {
    drawPixel(cx - i, cy - half + i);
    drawPixel(cx + i, cy - half + i);
    drawPixel(cx - i, cy + half - i);
    drawPixel(cx + i, cy + half - i);
  }
}

void graphic() {
  int cx = 95;   // Center x (adjust as needed)
  int cy = 32;   // Center y (adjust as needed)
  int size = 30; // Size of shape (adjust as needed)
 
  if (DATETIME.sec & 1) {
    drawDiamond(cx, cy, 20);
    drawDiamond(cx, cy, 30);
  } else {
    drawDiamond(cx, cy, 30);
    drawDiamond(cx, cy, 40);
  }
}

void setupLowPower() {
  ADCSRA &= ~(1 << ADEN);               // Disable ADC
  ACSR |= (1 << ACD);                   // Disable analog comparator
  PRR = (1 << PRTIM1) | (1 << PRADC);   // Power down Timer1 and ADC
  set_sleep_mode(SLEEP_MODE_IDLE);      // IDLE mode (Timer0 keeps running)
  sleep_enable();                       // Enable sleep
}

int main() {
  // Calibrate oscillator (chip-specific, tune by observation)
  OSCCAL = 92;

  // Setup Timer0 in CTC mode, prescaler /64, compare at 125
  // 8MHz/8/64/125 = 125Hz, so 125 interrupts = 1 second
  TCCR0A = (1 << WGM01);
  TCCR0B = (TCCR0B & 0xF8) | 0x03;
  TIMSK = (1 << OCIE0A);
  OCR0A = 125;

  // Enable pin change interrupts
  GIMSK = (1 << PCIE);

  // Setup PB3 - selector button with pull-up
  PCMSK |= (1 << PCINT3);
  DDRB &= ~(1 << DDB3);
  PORTB |= (1 << PORTB3);

  // Setup PB4 - setter button with pull-up
  PCMSK |= (1 << PCINT4);
  DDRB &= ~(1 << DDB4);
  PORTB |= (1 << PORTB4);

  // Setup PB1 - wake button with pull-up
  PCMSK |= (1 << PCINT1);
  DDRB &= ~(1 << DDB1);
  PORTB |= (1 << PORTB1);

  setupLowPower();

  sei();  // Enable global interrupts

  // Initialize OLED
  oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);  
  oled.clear();
  oled.setContrast(0x01);

  DISPLAY_STATE.on(); 

  // Main loop
  while (1) {
    // Handle 1-second tick
    if (TICKFLAG) {
      TICKFLAG = 0;
      DATETIME.tick();          
      DISPLAY_STATE.tick();

      // Manage display state
      if (DISPLAY_STATE.isOn) {
        displayOn();
        updateDisplay();
      } else {
        displayOff();
      }
    }

    // Handle selector button
    if (SELECTOR_PRESS) {
      updateSelector();
      DISPLAY_STATE.on();       
    }

    // Handle setter button
    if (SETTER_PRESS) {
      setDateTime();
      DISPLAY_STATE.on();       
    }

    sleep_mode();  // Sleep until next interrupt (saves ~0.35mA)
  }
}
