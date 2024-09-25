#ifndef _PINOUT_H
#define _PINOUT_H
#include<Arduino.h>

struct PINS {
  static const uint8_t LEFT_PWM = D0;
  static const uint8_t LEFT_IN1 = D1;
  static const uint8_t LEFT_IN2 = D2;

  static const uint8_t RIGHT_PWM = D5;
  static const uint8_t RIGHT_IN1 = D3;
  static const uint8_t RIGHT_IN2 = D4;

  static const uint8_t LEFT_INDICATOR = D7;
  static const uint8_t RIGHT_INDICATOR = D8;

  static const uint16_t IRRECV = D6;

  void init() const {
    pinMode(LEFT_PWM, OUTPUT);
    pinMode(LEFT_IN1, OUTPUT);
    pinMode(LEFT_IN2, OUTPUT);

    pinMode(RIGHT_PWM, OUTPUT);
    pinMode(RIGHT_IN1, OUTPUT);
    pinMode(RIGHT_IN2, OUTPUT);

    pinMode(LEFT_INDICATOR, OUTPUT);
    pinMode(RIGHT_INDICATOR, OUTPUT);
  }
};

#endif