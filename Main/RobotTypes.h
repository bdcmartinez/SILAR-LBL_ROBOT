#pragma once

struct StepData {
  int x = 0;
  int y = 0;
  int minutes = 0;
  int seconds = 0;
};

struct EncoderState {
  int xIndex = 0;
  int yIndex = 0;
  bool selectPressed = false;
  bool backPressed = false;
};
