#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/eeprom.h>
#include <TinyWireM.h>
#include <Tiny4kOLED.h>
#include "font16x32digits.h"
#include "font11x16.h"

// Interrupt flags
volatile uint8_t TICKFLAG = 0;
volatile uint8_t SELECTOR_PRESS = 0;
volatile uint8_t SETTER_PRESS = 0;
volatile uint8_t SELECTOR_DEBOUNCE = 0;
volatile uint8_t SETTER_DEBOUNCE = 0;

const uint8_t DISPLAY_TIMEOUT = 30;  // Seconds before display turns off
const uint8_t SELECTOR_MAX = 0x0F;
uint8_t SELECTOR = SELECTOR_MAX;
char MSG[9] = "CYPRESS.";
const char* const WEEKDAYS[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
unsigned int VCC_MILLIVOLTS = 0;

// EEPROM Addresses
const uint8_t* EEPROM_SIG_ADDR = 0;
const uint8_t* EEPROM_DATA_ADDR = 1;
const uint8_t EEPROM_SIG_VALUE = 0x42;

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

  bool tick() {
    // Increment seconds and cascade through time units
    bool hourPassed = false;
    if (++sec == 60) {
      sec = 0;
      min++;
    }

    if (min == 60) {
      hour++;
      min = 0;
      hourPassed = true;
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
    return hourPassed;
  }
};

DateTime DATETIME = {25, 12, 31, 23, 59, 50};

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
  SELECTOR = (SELECTOR + 1) & SELECTOR_MAX;  // Modulo 8 using bitwise AND
  SELECTOR_PRESS = 0;
  selectField();
}

void selectField() {
  uint8_t cx, cy;
  uint8_t flen = 12;
  static uint8_t prevSelector = SELECTOR_MAX;

  uint8_t Y2 = 28;
  // Determine cursor position for each field
  switch (SELECTOR) {
    case 0: cx = 9;  cy = 9; flen = 12; break;  // Month
    case 1: cx = 26; cy = 9; flen = 12; break;  // Day
    case 2: cx = 44; cy = 9; flen = 24; break;  // Year
    case 3: cx = 9;  cy = Y2; flen = 16; break;  // Hour
    case 4: cx = 42; cy = Y2; flen = 16; break;  // Minute
    case 5: cx = 75; cy = Y2; flen = 16; break;  // Second
    case 6: cx = 9; cy = 30; flen = 18; break; // weekday
    case 7: cx = 60; cy = 30; flen = 18; break; // OSCCAL value
    case 8: cx = 8; cy = 47; flen = 6; break;
    case 9: cx = 15; cy = 47; flen = 6; break;
    case 10: cx = 21; cy = 47; flen = 6; break;
    case 11: cx = 27; cy = 47; flen = 6; break;
    case 12: cx = 33; cy = 47; flen = 6; break;
    case 13: cx = 39; cy = 47; flen = 6; break;
    case 14: cx = 45; cy = 47; flen = 6; break;
    case 15: cx = 51; cy = 47; flen = 6; break;
    default:
      break;  // No selection
  }

  if (SELECTOR != prevSelector) {
    // Clear previous highlight
    oled.setCursor(9, 9);
    oled.setFont(FONT6X8);
    oled.clearToEOL();
    
    oled.setCursor(9, Y2);
    oled.setFont(FONT6X8);
    oled.clearToEOL();

    oled.setCursor(9, 30);
    oled.setFont(FONT6X8);
    oled.clearToEOL();

    prevSelector = SELECTOR;

    // Draw new highlight
    if (SELECTOR < 16) {
      oled.setCursor(cx, cy);
      oled.fillLength(0x01, flen);
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
      break;

    case 7:      
      if (++OSCCAL == 0XFF) {
        OSCCAL = 0;
      }
      break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15: 
      processMsg();
      break;
    default:
      break;
  }
}

void processMsg() {
  if ((SELECTOR < 8) || (SELECTOR > 15)) {
    return;
  }
  uint8_t idx = SELECTOR - 8;
  char c = MSG[idx];

  if (c != '.' && (c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9')) {
    c = '.';
  }

  // Cycle: space → A..Z → a..z → 0..9 → space (only these characters possible)
  if (c == '.') {
    c = 'A';
  } else if (c == 'Z') {
    c = 'a';
  } else if (c == 'z') {
    c = '0';
  } else if (c == '9') {
    c = '.';
  } else {
    c++;
  }

  MSG[idx] = c;
}

void displayOn() {  
  oled.on();
}

void displayOff() {  
  oled.off(); 
  SELECTOR = SELECTOR_MAX;
  selectField();
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

  // Display date
  oled.setCursor(8, 0);
  oled.setFont(FONT6X8);
  oled.print(datebuf);

  // Display time
  oled.setCursor(8, 10);
  // oled.setFont(FONT8X16);
  // oled.setFont(FONT16X32DIGITS);
  oled.setFont(FONT11X16);
  oled.print(timebuf);

  // Display Weekday
  oled.setFont(FONT6X8);
  oled.setCursor(8, 29);    
  oled.print(WEEKDAYS[p->weekday]);
  
  // Show calibration value
  oled.setCursor(36, 29);
  oled.setFont(FONT6X8);

  oled.print("CLK:");
  if (OSCCAL < 10) {
    oled.print("00");
  } else if ((OSCCAL >= 10) && (OSCCAL <= 99)) {
    oled.print("0");
  }  
  oled.print(OSCCAL);   

  /* Display Name */
  oled.setFont(FONT6X8);
  oled.setCursor(8, 47);
  oled.print(MSG);

  /* Display battery millivolts */
  oled.setCursor(60, 47);
  oled.print("BAT:");
  oled.print(VCC_MILLIVOLTS);
}

void setupLowPower() {
  ADCSRA &= ~(1 << ADEN);               // Disable ADC
  ACSR |= (1 << ACD);                   // Disable analog comparator
  PRR = (1 << PRTIM1) | (1 << PRADC);   // Power down Timer1 and ADC
  set_sleep_mode(SLEEP_MODE_IDLE);      // IDLE mode (Timer0 keeps running)
  sleep_enable();                       // Enable sleep
}

void saveState() {
  // write signature byte
  eeprom_update_byte(EEPROM_SIG_ADDR, EEPROM_SIG_VALUE);
  // write state block
  eeprom_update_block((const void*)&DATETIME, (void*)EEPROM_DATA_ADDR, sizeof(DATETIME));
}

void loadState() {
    uint8_t sign = eeprom_read_byte((uint8_t*)EEPROM_SIG_ADDR);
    // Only load if the signature matches
    if (sign == EEPROM_SIG_VALUE) {
        eeprom_read_block(
          (void*)&DATETIME, 
          (const void*)EEPROM_DATA_ADDR, 
          sizeof(DATETIME)
        );
    }
}

void readVcc() {
  /*
    Primitive battery status code; simply reports the supply voltage.
    Since a discharging battery loses its voltage, a reduction in supply
    voltage is equivalent to battery discharge. Supply voltage of a 
    new CR2032 battery is about 3.3 volts. A reading near 2.9 indicates 
    a low battery. 

    We have powered down the ADC to save battery. So we enable the ADC, 
    take the reading and disable the ADC again.
  */

  // 1. Power up the ADC
  PRR &= ~(1 << PRADC);    // Power Reduction Register: Disable ADC power-down
  ADCSRA |= (1 << ADEN);   // Enable the ADC
  
  // 2. Configure for Internal 1.1V measurement
  #if defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #endif

  // 3. Short delay for the reference voltage to stabilize
  for (volatile uint16_t i = 0; i < 250; i++); 

  // 4. Take the measurement
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); 

  // 5. Calculate Millivolts.   
  VCC_MILLIVOLTS = (unsigned int)(1125300L / ADC); 

  // 6. Shut it back down to save battery
  ADCSRA &= ~(1 << ADEN);  // Disable ADC
  PRR |= (1 << PRADC);     // Return to Power Reduction Mode
}

int main() {
  // Calibrate oscillator (chip-specific, tune by observation)
  OSCCAL = 92;

  // Setup Timer0 in CTC mode, prescaler /64, compare at 125
  // 1MHz/(64 * 125) = 125Hz, so 125 interrupts = 1 second
  TCCR0A = (1 << WGM01);
  TCCR0B = (TCCR0B & 0xF8) | 0x03;
  TIMSK = (1 << OCIE0A);
  OCR0A = 124;

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
  loadState();
  sei();  // Enable global interrupts

  // Initialize OLED
  oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);  
  oled.clear();
  oled.setContrast(0x01);

  // Read the battery voltage on power On
  readVcc();

  // Power on the display
  DISPLAY_STATE.on(); 

  bool hourPassed;
  // Main loop
  while (1) {
    // Handle 1-second tick
    if (TICKFLAG) {
      TICKFLAG = 0;
      hourPassed = DATETIME.tick();
      if (hourPassed) {
        saveState();
        readVcc();        
      }
       
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
