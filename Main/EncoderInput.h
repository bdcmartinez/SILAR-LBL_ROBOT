#pragma once

#include <Arduino.h>
#include "RobotTypes.h"

class EncoderInput {
 public:
  EncoderInput(int clkA, int dtA, int buttonA, int clkB, int dtB, int buttonB);

  void begin();
  EncoderState read();
  void setLimits(int maxX, int maxY);

 private:
  int clkA_;
  int dtA_;
  int buttonA_;
  int clkB_;
  int dtB_;
  int buttonB_;

  int lastClkA_ = HIGH;
  int lastClkB_ = HIGH;

  EncoderState state_;
  int maxX_ = 100;
  int maxY_ = 100;

  unsigned long lastButtonReadMs_ = 0;
};
