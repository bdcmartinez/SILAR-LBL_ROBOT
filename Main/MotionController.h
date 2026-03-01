#pragma once

#include <Arduino.h>

class MotionController {
 public:
  MotionController(int dirX, int pulX, int dirY, int pulY, int pulseDelayUs);

  void begin();
  void moveTo(int targetXSteps, int targetYSteps);

  int xSteps() const { return currentXSteps_; }
  int ySteps() const { return currentYSteps_; }

 private:
  int dirX_;
  int pulX_;
  int dirY_;
  int pulY_;
  int pulseDelayUs_;

  int currentXSteps_ = 0;
  int currentYSteps_ = 0;

  void stepAxis(int dirPin, int pulPin, bool dir, int pulses);
};
