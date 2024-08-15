const uint8_t mA1 = D2;
const uint8_t mA2 = D3;
const uint8_t mB1 = D5;
const uint8_t mB2 = D6;
const uint8_t mEN = D1;

void setup() {

  pinMode(mA1, OUTPUT);
  pinMode(mA2, OUTPUT);
  pinMode(mB1, OUTPUT);
  pinMode(mB2, OUTPUT);
  pinMode(mEN, OUTPUT);

  // drive the motor
  analogWrite(mEN, 100);

  
}

void loop() {
  //A FWD
  digitalWrite(mA1, LOW);
  digitalWrite(mA2, HIGH);  

  // B FWD
  digitalWrite(mB1, LOW);
  digitalWrite(mB2, HIGH);
  delay(500);

  //A REV
  digitalWrite(mA1, HIGH);
  digitalWrite(mA2, LOW);  

  // B REV
  digitalWrite(mB1, HIGH);
  digitalWrite(mB2, LOW);
  delay(500);

}