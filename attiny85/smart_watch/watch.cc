#include <avr/io.h>
#include <avr/interrupt.h>
#include <TinyWireM.h>
#include <Tiny4kOLED.h>

/* GLOBALS */
volatile uint8_t TICKFLAG = 0;
volatile uint8_t SELECTOR_PRESS = 0;
volatile uint8_t SETTER_PRESS = 0;
volatile uint8_t SELECTOR_DEBOUNCE = 0;
volatile uint8_t SETTER_DEBOUNCE = 0;

uint8_t SELECTOR = 6;
uint8_t DISPLAY_TIMEOUT = 30;

/* datetime */
struct DateTime {
  uint8_t year; // only last two digits
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t min;
  uint8_t sec; 

  uint8_t daysInMonth() {      
    switch (month) {
      case 4: case 6: case 9: case 11:
        return 30;
        break;
      case 2:
        /* fast divisibility check by 4: if (year & 0x03) == 0 
        (i.e. false), then year is div by 4 */
        return (year & 0x03) ? 28 : 29;
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

    if (min == 60) {    
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

DateTime DATETIME{25, 11, 2, 12, 1, 0};

/* Timed display with state and countdown */
struct TimedDisplayState {
  bool isOn;
  uint8_t timer;

  void tick() {
    if (timer > 0) {
      timer--;
    }

    if (timer == 0) {
      isOn = false;
    }
  }

  void on() {
    isOn = true;
    timer = DISPLAY_TIMEOUT;
  }
};

/* DISPLAY_STATE is changed in an ISR */
volatile TimedDisplayState DISPLAY_STATE{true, DISPLAY_TIMEOUT};

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

  /* handle pin 1 interrupt */
  if (!(PINB & _BV(PB1))) {
    DISPLAY_STATE.on();
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
  display();
}

void display() {
  if (!DISPLAY_STATE.isOn) {
    oled.off();
    return;
  }

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
  oled.on();
}

int main() {  
  /* Tune down OSCCAL since the clock is running faster 
  The value 40 was arrived at by trial and error. Every
  chip will have its own unique value. Chips will have 
  about 10% timing error according to spec. We can think
  about providing a user-adjustable POT to tune this value.
  */
  if (OSCCAL > 40) {
    OSCCAL -= 40;
  }
  
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

  PCMSK |= _BV(PCINT4); 
  DDRB &= ~_BV(DDB4); 
  PORTB |= _BV(PORTB4); 

  /* Set up PB1 as a WAKE UP button */
  PCMSK |= _BV(PCINT1);
  DDRB &= ~_BV(DDB1); 
  PORTB |= _BV(PORTB1); 

  /* Set global interrupt flag in SREG */
  sei();

  /* Set up OLED display */
  oled.begin(128, 64, sizeof(tiny4koled_init_128x64br), tiny4koled_init_128x64br);
  oled.setFont(FONT6X8);
  oled.clear();   
  oled.setContrast(0x02);       
  oled.off();
  oled.on();
  oled.setCursor(32, 22);
  oled.print(OSCCAL);  

  /* Main timing loop */
  while (1) {                 
    if (TICKFLAG == 1) {      
      TICKFLAG = 0;
      DATETIME.tick();   
      DISPLAY_STATE.tick();
      display();
    }

    if (SELECTOR_PRESS) {      
      updateSelector();  
      DISPLAY_STATE.on();                                 
    }

    if (SETTER_PRESS) {
      setDateTime();
      DISPLAY_STATE.on();
    }
  }  
}    
