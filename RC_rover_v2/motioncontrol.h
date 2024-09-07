#ifndef _MOTIONCONTROL_H
#define _MOTIONCONTROL_H

#include "pinout.h"

enum class Side {LEFT, RIGHT};
enum class Direction {FWD, REV};

class MotionControl {  
  const PINS& pins;

  const uint8_t minSpeedLevel = 1;
  const uint8_t maxSpeedLevel = 9;
  const uint8_t minSpeed = 0;
  const uint8_t maxSpeed = 255;
  const double levelToSpeedFactor = double(maxSpeed - minSpeed) / double(maxSpeedLevel - minSpeedLevel);  
  const uint8_t speedStep = static_cast<uint8_t>(levelToSpeedFactor);

  uint8_t leftSpeed;
  uint8_t rightSpeed;

  public: 
    // ctor
    MotionControl(const PINS& p);

    // Speed -- low level operations
    void setLeftSpeed(uint8_t value);
    void setRightSpeed(uint8_t value);
    void setSpeed(uint8_t leftSpeed, uint8_t rightSpeed);
    void setSpeed(uint8_t value);

    // Speed -- high level operations
    void setSpeedLevel(uint8_t level);       
    void incrSpeed();
    void decrSpeed();
    void stop();

    // Steering -- low level operations (differential steering)
    void setDirection(Side side, Direction dir);
    void leftFwd();
    void leftReverse();
    void rightFwd();
    void rightReverse();
    void fwd();
    void reverse();

    // Steering -- high level operations
    void turnLeft();
    void turnRight();

    // Indicators
    void leftIndicatorOn();
    void leftIndicatorOff();
    void rightIndicatorOn();
    void rightIndicatorOff();
};

#endif
