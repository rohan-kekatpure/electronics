#include <Arduino.h>

// Pin definitions
const int PDATA = D7; // DS (Data) pin of 74HC595
const int PCLOCK = D5; // SH_CP (Clock) pin of 74HC595
const int PLATCH = D8; // ST_CP (Latch) pin of 74HC595

unsigned _HH = 0, _SS = 0, _T;
volatile unsigned _MM = 0;
unsigned long start  = millis();
unsigned long current = start;
unsigned period = 1000;  

volatile unsigned long lastUpdateMM = 0;
volatile unsigned long lastUpdateHH = 0;
unsigned debounceDelay = 200;

struct Digit {
  unsigned value;
  const unsigned selector;
  Digit(): value{0}, selector{0} {}
  Digit(unsigned value, unsigned selector): value{value}, selector{selector} {}  
};

struct Display {
  Digit d1, d2, d3, d4;

  Display(Digit d1, Digit d2, Digit d3, Digit d4): 
    d1{d1}, d2{d2}, d3{d3}, d4{d4} {}

  void set(unsigned value) {
    d4.value = value % 10;    
    value /= 10;
    d3.value = value % 10;
    value /= 10;
    d2.value = value % 10;
    value /= 10;
    d1.value = value % 10;
  }  
};

Digit H1{0, D0}, H2{0, D1}, M1{0, D2}, M2{0, D3};
Display _DISPLAY{H1, H2, M1, M2};

// Segment patterns (common cathode)
const byte DIGIT_PATTERN[10] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111  // 9
};

void sendByte(byte data) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(PDATA, (data >> (7 - i)) & 0x01);
    digitalWrite(PCLOCK, HIGH);
    digitalWrite(PCLOCK, LOW);
  }
}

void flashDigit(const Digit& d, const unsigned duration) {
  // Push the value
  digitalWrite(PLATCH, LOW);
  sendByte(DIGIT_PATTERN[d.value]);
  digitalWrite(PLATCH, HIGH);

  // Enable the selector
  digitalWrite(d.selector, LOW);  

  // Persist the digit for a short duration, multiplexing
  delay(duration);
  digitalWrite(d.selector, HIGH);
}

void printDigit(const Digit& d) {
  Serial.print("value=");
  Serial.print(d.value);
  Serial.print(", selector=");
  Serial.print(d.selector);
  Serial.println();
}

void cycle(const Display& display) {
  const unsigned duration = 1;    
  flashDigit(display.d1, duration);
  flashDigit(display.d2, duration);
  flashDigit(display.d3, duration);  
  flashDigit(display.d4, duration);
}

void setup() {
  Serial.begin(115200);
  pinMode(PDATA, OUTPUT);
  pinMode(PCLOCK, OUTPUT);
  pinMode(PLATCH, OUTPUT);

  // Set all selectors to output mode
  pinMode(H1.selector, OUTPUT);
  pinMode(H2.selector, OUTPUT);
  pinMode(M1.selector, OUTPUT);
  pinMode(M2.selector, OUTPUT);

  // disable all digits
  digitalWrite(H1.selector, HIGH);
  digitalWrite(H2.selector, HIGH);
  digitalWrite(M1.selector, HIGH);
  digitalWrite(M2.selector, HIGH);

  // Attach interrupts to HH and MM increment pins
  uint8_t MM_INT = digitalPinToInterrupt(D4);
  attachInterrupt(MM_INT, ISR_incrementMinute, FALLING);
}

IRAM_ATTR void ISR_incrementMinute() {  
  unsigned long now = millis();
  if ((now - lastUpdateMM) > debounceDelay) {
    lastUpdateMM = now;
    _MM = _MM == 59? 0 : _MM + 1;      
  }
}

void loop() {  
  _T = 100 * _HH + _MM;
  while (current - start < period) {
    _DISPLAY.set(_T);
    cycle(_DISPLAY);  
    current = millis();
  }
  start = current; 
  if (++_SS == 60) {
    _SS = 0;
    ++_MM;
  }

  if (_MM == 60) {
    _MM = 0;
    ++_HH;
  }

  if (_HH == 24) {
    _HH = 0;
  }
}