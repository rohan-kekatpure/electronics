#include <Arduino.h>

// Pin definitions
const uint8_t PDATA = D7; // DS (Data) pin of 74HC595
const uint8_t PCLOCK = D5; // SH_CP (Clock) pin of 74HC595
const uint8_t PLATCH = D8; // ST_CP (Latch) pin of 74HC595

const uint8_t MM_SET = D4; // MM set pin
const uint8_t HH_SET = D6; // HH set pin

const unsigned SECOND = 1000;
const unsigned MINUTE = 60 * SECOND;
const unsigned debounceDelay = 250;
const unsigned _12HMODE = true; // Make false for 24H clock

unsigned _HH, _MM, DISPLAY_HH;
unsigned MMSetTime, HHSetTime;
unsigned long now, minuteStart;

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
    // 0bPGFEDCBA
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

void flashDigit(const Digit& d, const unsigned duration, const bool showDP=false) {
  byte ptrn = DIGIT_PATTERN[d.value]; 

  // Add decimal point (set MSB) if showDP is true
  if (showDP) {
    ptrn |= 0b10000000;
  }

  // Push the value
  digitalWrite(PLATCH, LOW);
  sendByte(ptrn);
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
  const unsigned duration = 2;    
  flashDigit(display.d1, duration);
  flashDigit(display.d2, duration, true);
  flashDigit(display.d3, duration);  
  flashDigit(display.d4, duration, _HH > 12); // Indicate PM hours by a DP
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

  // Set MM and HH set pins to INPUT
  pinMode(MM_SET, INPUT_PULLUP);
  pinMode(HH_SET, INPUT_PULLUP);

  // disable all digits
  digitalWrite(H1.selector, HIGH);
  digitalWrite(H2.selector, HIGH);
  digitalWrite(M1.selector, HIGH);
  digitalWrite(M2.selector, HIGH);

  // Initialize timings
  _HH = 0;
  _MM = 0;
  now = millis();
  minuteStart = now;

  // Initialize HH and MM set time
  MMSetTime = 0;
  HHSetTime = 0;
}

void loop() {
  now = millis();  
  if (now - minuteStart > MINUTE) {
    minuteStart = now;
    _MM++;
  }    

  // Reset hour at 60 minutes
  if (_MM >= 60) {
    _MM = 0;
    _HH++;
  }      

  // Reset day at 24 hours
  if (_HH >= 24) {
    _HH = 0;
  }

  if (_12HMODE && (_HH > 12)) {
    DISPLAY_HH = _HH - 12;
  } else {
    DISPLAY_HH = _HH;
  }

  _DISPLAY.set(100 * DISPLAY_HH + _MM);
  cycle(_DISPLAY);  

  // Minute increment
  if ((digitalRead(MM_SET) == LOW) && (now - MMSetTime > debounceDelay)) {
    _MM++;
    MMSetTime = now;
  } 

    // Hour increment
  if ((digitalRead(HH_SET) == LOW) && (now - HHSetTime > debounceDelay)) {
    _HH++;
    HHSetTime = now;
  } 
}