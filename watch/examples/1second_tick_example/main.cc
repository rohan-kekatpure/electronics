#include<avr/io.h>
#include<avr/interrupt.h>
#include<util/atomic.h>

volatile uint8_t COUNT = 0;
ISR(TIMER0_COMPA_vect) {
  COUNT++;  
}

int main() {  
  uint8_t current_count;
  DDRB |= (1 << PB0); 
  PORTB &= ~(1 << PB0);
  TCCR0A |= (1 << WGM01);
  TCCR0B = (TCCR0B & 0xF8) | 0x03;
  TIMSK |= 1 << OCIE0A;  
  OCR0A = 125;
  sei();

  while(1) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      current_count = COUNT;
    }

    if (current_count == 125) {
      PORTB ^= (1 << PB0);
      ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
        COUNT = 0;
      }
    }    
  }
}
