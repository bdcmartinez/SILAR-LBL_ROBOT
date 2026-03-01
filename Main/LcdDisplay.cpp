#include "LcdDisplay.h"

namespace {
String twoDigits(int value) {
  return value < 10 ? "0" + String(value) : String(value);
}
}

void LcdDisplay::begin() {
  lcd_.setBacklightPin(3, POSITIVE);
  lcd_.setBacklight(HIGH);
  lcd_.begin(20, 4);
  lcd_.clear();
}

void LcdDisplay::showSplash() {
  lcd_.clear();
  lcd_.setCursor(1, 1);
  lcd_.print("SILAR ROBOT READY");
  lcd_.setCursor(1, 2);
  lcd_.print("Refactor + UI flow");
}

void LcdDisplay::showMainMenu(int selected) {
  lcd_.clear();
  const char* options[3] = {"Start routine", "Edit steps", "Load from SD"};
  for (int i = 0; i < 3; ++i) {
    lcd_.setCursor(0, i);
    lcd_.print(i == selected ? ">" : " ");
    lcd_.print(options[i]);
  }
  lcd_.setCursor(0, 3);
  lcd_.print("A=select B=back");
}

void LcdDisplay::showStepEditor(const EncoderState& state, const StepData& draft, int stepCount) {
  lcd_.clear();
  lcd_.setCursor(0, 0);
  lcd_.print("Edit step #");
  lcd_.print(stepCount + 1);

  lcd_.setCursor(0, 1);
  lcd_.print("X:");
  lcd_.print(state.xIndex);
  lcd_.print(" Y:");
  lcd_.print(state.yIndex);

  lcd_.setCursor(0, 2);
  lcd_.print("T ");
  lcd_.print(twoDigits(draft.minutes));
  lcd_.print(":");
  lcd_.print(twoDigits(draft.seconds));

  lcd_.setCursor(0, 3);
  lcd_.print("A=save step B=done");
}

void LcdDisplay::showRunStatus(int layer, int step, int elapsedSeconds, int totalSeconds) {
  lcd_.setCursor(0, 0);
  lcd_.print("Running L:");
  lcd_.print(layer);
  lcd_.print(" S:");
  lcd_.print(step);
  lcd_.print("   ");

  lcd_.setCursor(0, 1);
  lcd_.print("Step t: ");
  lcd_.print(twoDigits(elapsedSeconds / 60));
  lcd_.print(":");
  lcd_.print(twoDigits(elapsedSeconds % 60));
  lcd_.print(" ");

  lcd_.setCursor(0, 2);
  lcd_.print("Total t:");
  lcd_.print(twoDigits(totalSeconds / 60));
  lcd_.print(":");
  lcd_.print(twoDigits(totalSeconds % 60));
  lcd_.print(" ");

  lcd_.setCursor(0, 3);
  lcd_.print("B to abort");
}

void LcdDisplay::showSaved(const String& fileName) {
  lcd_.clear();
  lcd_.setCursor(0, 1);
  lcd_.print("Saved routine:");
  lcd_.setCursor(0, 2);
  lcd_.print(fileName.substring(0, 20));
}

void LcdDisplay::showLoaded(const String& fileName, int stepCount) {
  lcd_.clear();
  lcd_.setCursor(0, 1);
  lcd_.print("Loaded:");
  lcd_.print(fileName.substring(0, 12));
  lcd_.setCursor(0, 2);
  lcd_.print("Steps: ");
  lcd_.print(stepCount);
}

void LcdDisplay::showFileBrowser(const std::vector<String>& files, int selected) {
  lcd_.clear();
  lcd_.setCursor(0, 0);
  lcd_.print("Choose file:");

  const int start = (selected / 3) * 3;
  for (int row = 0; row < 3; ++row) {
    const int idx = start + row;
    if (idx >= static_cast<int>(files.size())) break;

    lcd_.setCursor(0, row + 1);
    lcd_.print(idx == selected ? ">" : " ");
    lcd_.print(files[idx].substring(0, 19));
  }
}

void LcdDisplay::showError(const String& error) {
  lcd_.clear();
  lcd_.setCursor(0, 1);
  lcd_.print("ERROR:");
  lcd_.setCursor(0, 2);
  lcd_.print(error.substring(0, 20));
}
