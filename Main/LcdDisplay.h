#pragma once

#include <LiquidCrystal_I2C.h>
#include <vector>

#include "RobotTypes.h"

class LcdDisplay {
 public:
  explicit LcdDisplay(LiquidCrystal_I2C& lcd) : lcd_(lcd) {}

  void begin();
  void showSplash();
  void showMainMenu(int selected);
  void showStepEditor(const EncoderState& state, const StepData& draft, int stepCount);
  void showRunStatus(int layer, int step, int elapsedSeconds, int totalSeconds);
  void showSaved(const String& fileName);
  void showLoaded(const String& fileName, int stepCount);
  void showFileBrowser(const std::vector<String>& files, int selected);
  void showError(const String& error);

 private:
  LiquidCrystal_I2C& lcd_;
};
