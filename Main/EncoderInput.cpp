#include "EncoderInput.h"

namespace {
int clampInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}
}

EncoderInput::EncoderInput(int clkA, int dtA, int buttonA, int clkB, int dtB, int buttonB)
    : clkA_(clkA), dtA_(dtA), buttonA_(buttonA), clkB_(clkB), dtB_(dtB), buttonB_(buttonB) {}

void EncoderInput::begin() {
  pinMode(clkA_, INPUT_PULLUP);
  pinMode(dtA_, INPUT_PULLUP);
  pinMode(buttonA_, INPUT_PULLUP);
  pinMode(clkB_, INPUT_PULLUP);
  pinMode(dtB_, INPUT_PULLUP);
  pinMode(buttonB_, INPUT_PULLUP);

  lastClkA_ = digitalRead(clkA_);
  lastClkB_ = digitalRead(clkB_);
}

void EncoderInput::setLimits(int maxX, int maxY) {
  maxX_ = maxX;
  maxY_ = maxY;
}

EncoderState EncoderInput::read() {
  const int clkA = digitalRead(clkA_);
  const int clkB = digitalRead(clkB_);

  if (clkA != lastClkA_ && clkA == LOW) {
    const bool clockwise = digitalRead(dtA_) == HIGH;
    state_.xIndex += clockwise ? 1 : -1;
  }

  if (clkB != lastClkB_ && clkB == LOW) {
    const bool clockwise = digitalRead(dtB_) == HIGH;
    state_.yIndex += clockwise ? 1 : -1;
  }

  state_.xIndex = clampInt(state_.xIndex, 0, maxX_);
  state_.yIndex = clampInt(state_.yIndex, 0, maxY_);

  lastClkA_ = clkA;
  lastClkB_ = clkB;

  const unsigned long now = millis();
  if (now - lastButtonReadMs_ > 120) {
    state_.selectPressed = digitalRead(buttonA_) == LOW;
    state_.backPressed = digitalRead(buttonB_) == LOW;
    lastButtonReadMs_ = now;
  } else {
    state_.selectPressed = false;
    state_.backPressed = false;
  }

  return state_;
}
