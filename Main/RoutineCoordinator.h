#pragma once

#include <RTClib.h>
#include <vector>

#include "EncoderInput.h"
#include "LcdDisplay.h"
#include "MotionController.h"
#include "SdDataStore.h"

class RoutineCoordinator {
 public:
  RoutineCoordinator(LcdDisplay& lcd,
                     EncoderInput& encoder,
                     SdDataStore& dataStore,
                     MotionController& motion,
                     RTC_DS3231& rtc);

  void begin();
  void update();

 private:
  enum class ScreenState {
    MainMenu,
    Editing,
    BrowsingFiles,
    Running
  };

  LcdDisplay& lcd_;
  EncoderInput& encoder_;
  SdDataStore& dataStore_;
  MotionController& motion_;
  RTC_DS3231& rtc_;

  ScreenState screen_ = ScreenState::MainMenu;
  std::vector<StepData> steps_;
  std::vector<String> routineFiles_;

  int layerCount_ = 1;
  int menuIndex_ = 0;
  int fileIndex_ = 0;
  StepData draftStep_;

  void runRoutine();
  void saveCurrentStepFromEncoder(const EncoderState& state);
  void handleMainMenu(const EncoderState& state);
  void handleEditing(const EncoderState& state);
  void handleFileBrowsing(const EncoderState& state);
};
