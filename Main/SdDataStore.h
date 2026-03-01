#pragma once

#include <Arduino.h>
#include <RTClib.h>
#include <vector>

#include "SD.h"
#include "RobotTypes.h"

class SdDataStore {
 public:
  SdDataStore(int chipSelect, RTC_DS3231& rtc);

  bool begin();
  bool loadLastRoutine(std::vector<StepData>& steps);
  bool loadRoutine(const String& name, std::vector<StepData>& steps);
  bool saveRoutine(const std::vector<StepData>& steps, String& outFileName);
  bool listRoutineNames(std::vector<String>& names);

  bool loadMotorPosition(int& xSteps, int& ySteps);
  bool saveMotorPosition(int xSteps, int ySteps);

 private:
  int chipSelect_;
  RTC_DS3231& rtc_;

  const char* positionFile_ = "/pos.txt";
  const char* selectedFile_ = "/selected.txt";

  String nowFileName() const;
  bool readSelectedRoutineName(String& name);
  bool writeSelectedRoutineName(const String& name);
};
