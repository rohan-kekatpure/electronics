#ifndef _MOTIONCONTROL_H
#define _MOTIONCONTROL_H

#include<Arduino.h>
#include "pinout.h"
#include "trip.h"

enum class Side {LEFT, RIGHT};
enum class Direction {FWD, REV};
enum class Turn {LEFT, RIGHT};

class MotionControl {  
  const PINS& pins;

  /*
    Pins that control left and right side of the car are 
    not static constants, but change based on which direction
    of the car is defined as front. For reverse motion, left
    and right side control pins will be swapped
  */
  // power pins
  uint8_t LPWM, RPWM;

  // rotation direction pins
  uint8_t LIN1, LIN2, RIN1, RIN2; 

  public:

  // Motion mode
  Direction mode;

  const uint8_t minSpeedLevel = 1;
  const uint8_t maxSpeedLevel = 9;
  const uint8_t minSpeed = 0;
  const uint8_t maxSpeed = 255;  

  uint8_t leftSpeed;
  uint8_t rightSpeed;
  uint8_t leftSpeedLevel;
  uint8_t rightSpeedLevel;

  public: 
    // ctor
    MotionControl(const PINS& p);

    // motion modes    
    void setModeFwd();
    void setModeRev();
    void toggleMode();

    //Left and right pin assignment depends on fwd or rev motion mode
    void pinConfigFwd();
    void pinConfigRev();

    // Speed -- low level operations
    void setLeftSpeed(uint8_t value);
    void setRightSpeed(uint8_t value);
    void setSpeed(uint8_t leftSpeed, uint8_t rightSpeed);
    void setSpeed(uint8_t value);
    void setSpeedLevel(uint8_t level);       
    void disable();
    void restore();

    // Speed -- high level operations    
    void incrSpeed();
    void decrSpeed();
    void stop();
    void backup();

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
    void turnMoveHalt(Turn dir);

    // Steering tricks
    void goCircle(uint8_t radius);
    void goSnake();
    void goRandomDirection();
    void goSpiral();
    void goSquare();

    // Indicators
    void leftIndicatorOn();
    void leftIndicatorOff();
    void rightIndicatorOn();
    void rightIndicatorOff();

    // Executing Moves
    void execMove(const Move& move);
    void execTrip(const Trip& trip);
};

#endif
