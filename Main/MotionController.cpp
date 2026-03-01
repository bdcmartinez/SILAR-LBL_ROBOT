#include "MotionController.h"

MotionController::MotionController(int dirX, int pulX, int dirY, int pulY, int pulseDelayUs)
    : dirX_(dirX), pulX_(pulX), dirY_(dirY), pulY_(pulY), pulseDelayUs_(pulseDelayUs) {}

void MotionController::begin() {
  pinMode(dirX_, OUTPUT);
  pinMode(pulX_, OUTPUT);
  pinMode(dirY_, OUTPUT);
  pinMode(pulY_, OUTPUT);
}

void MotionController::stepAxis(int dirPin, int pulPin, bool dir, int pulses) {
  digitalWrite(dirPin, dir ? HIGH : LOW);
  for (int i = 0; i < pulses; ++i) {
    digitalWrite(pulPin, HIGH);
    delayMicroseconds(pulseDelayUs_);
    digitalWrite(pulPin, LOW);
    delayMicroseconds(pulseDelayUs_);
  }
}

void MotionController::moveTo(int targetXSteps, int targetYSteps) {
  const int deltaY = targetYSteps - currentYSteps_;
  if (deltaY > 0) stepAxis(dirY_, pulY_, true, deltaY);
  if (deltaY < 0) stepAxis(dirY_, pulY_, false, -deltaY);
  currentYSteps_ = targetYSteps;

  const int deltaX = targetXSteps - currentXSteps_;
  if (deltaX > 0) stepAxis(dirX_, pulX_, true, deltaX);
  if (deltaX < 0) stepAxis(dirX_, pulX_, false, -deltaX);
  currentXSteps_ = targetXSteps;
}
