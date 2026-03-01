#include "RoutineCoordinator.h"

namespace {
int clampIndex(int value, int minV, int maxV) {
  if (value < minV) return minV;
  if (value > maxV) return maxV;
  return value;
}
}

RoutineCoordinator::RoutineCoordinator(LcdDisplay& lcd,
                                       EncoderInput& encoder,
                                       SdDataStore& dataStore,
                                       MotionController& motion,
                                       RTC_DS3231& rtc)
    : lcd_(lcd), encoder_(encoder), dataStore_(dataStore), motion_(motion), rtc_(rtc) {}

void RoutineCoordinator::begin() {
  lcd_.showSplash();

  int x = 0;
  int y = 0;
  dataStore_.loadMotorPosition(x, y);
  motion_.moveTo(x, y);

  dataStore_.loadLastRoutine(steps_);
  encoder_.setLimits(100, 100);
  delay(1000);
  lcd_.showMainMenu(menuIndex_);
}

void RoutineCoordinator::saveCurrentStepFromEncoder(const EncoderState& state) {
  StepData step;
  step.x = state.xIndex;
  step.y = state.yIndex;
  step.minutes = draftStep_.minutes;
  step.seconds = draftStep_.seconds;
  steps_.push_back(step);
}

void RoutineCoordinator::runRoutine() {
  if (steps_.empty()) {
    lcd_.showError("No steps loaded");
    delay(900);
    screen_ = ScreenState::MainMenu;
    return;
  }

  screen_ = ScreenState::Running;
  const uint32_t routineStart = rtc_.now().unixtime();

  for (int layer = 0; layer < layerCount_; ++layer) {
    for (size_t stepIndex = 0; stepIndex < steps_.size(); ++stepIndex) {
      const EncoderState cancelState = encoder_.read();
      if (cancelState.backPressed) {
        screen_ = ScreenState::MainMenu;
        return;
      }

      const StepData& step = steps_[stepIndex];
      const int targetX = step.x * 150;
      const int targetY = step.y * 100;

      motion_.moveTo(targetX, targetY);
      dataStore_.saveMotorPosition(motion_.xSteps(), motion_.ySteps());

      const uint32_t stepStart = rtc_.now().unixtime();
      const uint32_t stepDuration = static_cast<uint32_t>(step.minutes * 60 + step.seconds);

      while (rtc_.now().unixtime() - stepStart <= stepDuration) {
        const uint32_t now = rtc_.now().unixtime();
        lcd_.showRunStatus(layer + 1, stepIndex + 1, now - stepStart, now - routineStart);
        const EncoderState state = encoder_.read();
        if (state.backPressed) {
          screen_ = ScreenState::MainMenu;
          return;
        }
      }
    }
  }

  screen_ = ScreenState::MainMenu;
}

void RoutineCoordinator::handleMainMenu(const EncoderState& state) {
  menuIndex_ = clampIndex(state.xIndex % 3, 0, 2);
  lcd_.showMainMenu(menuIndex_);

  if (!state.selectPressed) return;

  if (menuIndex_ == 0) {
    runRoutine();
    return;
  }

  if (menuIndex_ == 1) {
    screen_ = ScreenState::Editing;
    draftStep_.minutes = state.yIndex / 60;
    draftStep_.seconds = state.yIndex % 60;
    return;
  }

  if (menuIndex_ == 2) {
    routineFiles_.clear();
    if (!dataStore_.listRoutineNames(routineFiles_)) {
      lcd_.showError("No routine files");
      delay(900);
      return;
    }
    fileIndex_ = 0;
    screen_ = ScreenState::BrowsingFiles;
  }
}

void RoutineCoordinator::handleEditing(const EncoderState& state) {
  draftStep_.minutes = clampIndex(state.yIndex / 60, 0, 59);
  draftStep_.seconds = clampIndex(state.yIndex % 60, 0, 59);
  lcd_.showStepEditor(state, draftStep_, static_cast<int>(steps_.size()));

  if (state.selectPressed) {
    saveCurrentStepFromEncoder(state);
    String fileName;
    if (dataStore_.saveRoutine(steps_, fileName)) {
      lcd_.showSaved(fileName);
      delay(800);
    } else {
      lcd_.showError("Save failed");
      delay(800);
    }
  }

  if (state.backPressed) {
    screen_ = ScreenState::MainMenu;
  }
}

void RoutineCoordinator::handleFileBrowsing(const EncoderState& state) {
  if (routineFiles_.empty()) {
    screen_ = ScreenState::MainMenu;
    return;
  }

  fileIndex_ = clampIndex(state.xIndex, 0, static_cast<int>(routineFiles_.size()) - 1);
  lcd_.showFileBrowser(routineFiles_, fileIndex_);

  if (state.selectPressed) {
    if (dataStore_.loadRoutine(routineFiles_[fileIndex_], steps_)) {
      lcd_.showLoaded(routineFiles_[fileIndex_], static_cast<int>(steps_.size()));
      delay(900);
      screen_ = ScreenState::MainMenu;
    } else {
      lcd_.showError("Load failed");
      delay(900);
    }
  }

  if (state.backPressed) {
    screen_ = ScreenState::MainMenu;
  }
}

void RoutineCoordinator::update() {
  const EncoderState state = encoder_.read();

  switch (screen_) {
    case ScreenState::MainMenu:
      handleMainMenu(state);
      break;
    case ScreenState::Editing:
      handleEditing(state);
      break;
    case ScreenState::BrowsingFiles:
      handleFileBrowsing(state);
      break;
    case ScreenState::Running:
      break;
  }
}
