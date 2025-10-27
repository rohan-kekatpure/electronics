#include <avr/io.h>
#include <avr/interrupt.h>
#include <TinyWireM.h>
#include <Tiny4kOLED.h>

volatile uint8_t TICKFLAG = 0;
volatile uint8_t SELECTOR_PRESS = 0;
volatile uint8_t SETTER_PRESS = 0;
volatile uint8_t SELECTOR_DEBOUNCE = 0;
volatile uint8_t SETTER_DEBOUNCE = 0;

uint8_t SELECTOR = 6;

/* datetime */

struct DateTime {
  uint8_t year; // only last two digits
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec; 

  uint8_t daysInMonth() {  
    bool isLeapYear = ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);   
    switch (month) {
      case 4: case 6: case 9: case 11:
        return 30;
        break;
      case 2:
        return isLeapYear ? 29 : 28;
        break;
      default:
        return 31;
    }    
  }

  void tick() {
    /* Advances 1 second and cascading down */
    if (++sec == 60) {
      sec = 0; 
      min++;    
    }

    if (month == 60) {    
      hour++;
      min = 0;    
    }

    if (hour == 24) {
      day++;
      hour = 0;
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

DateTime DATETIME{25, 10, 26, 20, 12, 0};

ISR(TIMER0_COMPA_vect) {
  static uint8_t COUNT = 0;
  COUNT++;  
  if (COUNT == 125) {
    COUNT = 0;
    TICKFLAG = 1;
  }

  if (SELECTOR_DEBOUNCE > 0) {
    SELECTOR_DEBOUNCE--;
  }

  if (SETTER_DEBOUNCE > 0) {
    SETTER_DEBOUNCE--;
  }
}

ISR(PCINT0_vect) { 
  /* handle pin 3 interrupt */
  if ( (!(PINB & _BV(PB3))) && (SELECTOR_DEBOUNCE == 0) ){    
      SELECTOR_PRESS = 1;
      SELECTOR_DEBOUNCE = 50;
  }  

  /* handle pin 4 interrupt */
  if ( (!(PINB & _BV(PB4))) && (SETTER_DEBOUNCE == 0) ){    
      SETTER_PRESS = 1;
      SETTER_DEBOUNCE = 50;
  }  
}

void updateSelector() {  
    /* 
    Efficient way to do `SELECTOR = (SELECTOR + 1) % 7`    
    but avoiding software division (theres no hardware 
    support for division on AVR)
    */    
    SELECTOR = (SELECTOR + 1) & 0x07;    
    SELECTOR_PRESS = 0; // release selector button    
    selectField();
}

void selectField() {    
  uint8_t cx, cy;
  uint8_t flen = 12;
  static uint8_t prevSelector = 0;  
  switch (SELECTOR) {
    case 0: cx = 9; cy = 10; flen = 12; break;
    case 1: cx = 26; cy = 10; flen = 12; break;
    case 2: cx = 44; cy = 10; flen = 24; break;
    case 3: cx = 9; cy = 20; flen = 12; break;
    case 4: cx = 26; cy = 20; flen = 12; break;
    case 5: cx = 44; cy = 20; flen = 12; break;
    case 6: case 7: default: break;      
  }
  
  if (SELECTOR != prevSelector) {    
    /* Clear previous highlighting */    
    oled.setCursor(9, 10);
    oled.clearToEOL();
    oled.setCursor(9, 20);
    oled.clearToEOL();
    prevSelector = SELECTOR;    

    /* New highlight */
    if (SELECTOR < 6) {
      oled.setCursor(cx, cy);
      oled.fillLength(0x0f, flen);    
    }
  }
}

void setDateTime() {
  SETTER_PRESS = 0;
  DateTime *p = &DATETIME;
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
      if (++p->sec > 59){
        p->sec = 0;
      }
      break;
    case 6: case 7: default: break;      
  }
}

void display() {
  DateTime *p = &DATETIME;  
  char datebuf[11];
  char timebuf[9];
  snprintf(
    datebuf, sizeof(datebuf), "%02d/%02d/20%d", 
    p->month, p->day, p->year
  );
  snprintf(
    timebuf, sizeof(timebuf), "%02d:%02d:%02d", 
    p->hour, p->min, p->sec
  );

  /* print buffers on OLED */
  oled.setCursor(8, 1);  
  oled.print(datebuf);
  oled.setCursor(8, 11);  
  oled.print(timebuf);
  oled.setCursor(8, 22);
  oled.print(SELECTOR); 
}

int main() {  
  /* Set up timer interrupt system to count 1 second */  
  TCCR0A |= _BV(WGM01);
  TCCR0B = (TCCR0B & 0xF8) | 0x03;
  TIMSK |= _BV(OCIE0A);  
  OCR0A = 125;

  /* Enable pin change interrupts */
  GIMSK |= _BV(PCIE); 

  PCMSK |= _BV(PCINT3); // Enable INTR on DP3
  DDRB &= ~_BV(DDB3); // DP3 as input
  PORTB |= _BV(PORTB3); // Pullup for DP3

  PCMSK |= _BV(PCINT4); // Enable INTR on DP4 
  DDRB &= ~_BV(DDB4); // DP4 as input  
  PORTB |= _BV(PORTB4); // Pullup for DP4
    
  /* Set global interrupt flag in SREG */
  sei();

  /* Set up OLED display */
  oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
  oled.setFont(FONT6X8);
  oled.clear();   
  oled.setContrast(0x02);       
  oled.off();
  oled.on();

  /* Main timing loop */
  while (1) {                 
    if (TICKFLAG == 1) {      
      TICKFLAG = 0;
      DATETIME.tick();
      display();
    }
    
    if (SELECTOR_PRESS) {
      updateSelector();                                   
    }

    if (SETTER_PRESS) {
      setDateTime();
    }
  }  
}    

