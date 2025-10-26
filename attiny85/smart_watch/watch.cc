#include <avr/io.h>
#include <avr/interrupt.h>
#include <TinyWireM.h>
#include <Tiny4kOLED.h>

volatile uint8_t TICKFLAG = 0;
volatile uint8_t SELECTOR_PRESS = 0;
volatile uint8_t SETTER_PRESS = 0;
volatile uint8_t DEBOUNCE_COUNTER = 0;

uint8_t SELECTOR = 6;

ISR(TIMER0_COMPA_vect) {
  static uint8_t COUNT = 0;
  COUNT++;  
  if (COUNT == 125) {
    COUNT = 0;
    TICKFLAG = 1;
  }

  if (DEBOUNCE_COUNTER > 0) {
    DEBOUNCE_COUNTER--;
  }
}

ISR(PCINT0_vect) { 
  if (DEBOUNCE_COUNTER == 0) {
    /* PINB3 high == PINB & (1 << PB3) */  
    if (!(PINB & (1 << PB3))) {
      SELECTOR_PRESS = 1;
      DEBOUNCE_COUNTER = 50;
    }
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
  switch (SELECTOR) {
    case 0:
      cx = 9;
      cy = 10;      
      flen = 12;
      break;
    case 1:
      cx = 27;
      cy = 10;  
      flen = 12;    
      break;
    case 2: 
      cx = 45;
      cy = 10;
      flen = 24;
      break;
    case 3: 
      cx = 9;
      cy = 20;
      flen = 12; 
      break;
    case 4: 
      cx = 27;
      cy = 20;
      flen = 12;
      break;
    case 5: 
      cx = 45;
      cy = 20;
      flen = 12;
      break;
    case 6:
    case 7:
    default:
      break;      
  }

  /* Clear previous highlighting */
  oled.setCursor(9, 10);
  oled.clearToEOL();
  oled.setCursor(9, 20);
  oled.clearToEOL();

  /* New highlight */
  if (SELECTOR < 6) {
    oled.setCursor(cx, cy);
    oled.fillLength(0x0f, flen);    
  }
}

void updateDateAndTime() {
  static uint8_t seconds = 50;
  static uint8_t minute = 37;
  static uint8_t hour = 9;
  static uint8_t day = 25;
  static uint8_t month = 10;
  static uint16_t year = 2025;

  /* Note that `main` call this function only when a second
  has elapsed. So everytime we're here, we have to update 
  `second` */   
  if (++seconds == 60) {
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

  /* Format data and time strings */
  char datebuf[11];
  char timebuf[9];
  snprintf(
    datebuf, sizeof(datebuf), "%02d/%02d/%d", 
    int(month), int(day), int(year)
  );
  snprintf(
    timebuf, sizeof(timebuf), "%02d:%02d:%02d", 
    int(hour), int(minute), int(seconds)
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
  TCCR0A |= (1 << WGM01);
  TCCR0B = (TCCR0B & 0xF8) | 0x03;
  TIMSK |= 1 << OCIE0A;  
  OCR0A = 125;

  /* Enable pin change interrupts */
  GIMSK |= (1 << PCIE); 

  PCMSK |= (1 << PCINT3); // Enable INTR on DP3
  DDRB &= ~(1 << DDB3); // DP3 as input
  PORTB |= (1 << PORTB3); // Pullup for DP3

  PCMSK |= (1 << PCINT4); // Enable INTR on DP4 
  DDRB &= ~(1 << DDB4); // DP4 as input  
  PORTB |= (1 << PORTB4); // Pullup for DP4
    
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
    if (SELECTOR_PRESS) {
      updateSelector();                                   
    }

    if (TICKFLAG == 1) {      
      TICKFLAG = 0;
      updateDateAndTime();      
    }
  }  
}    