#include "SdDataStore.h"

SdDataStore::SdDataStore(int chipSelect, RTC_DS3231& rtc) : chipSelect_(chipSelect), rtc_(rtc) {}

bool SdDataStore::begin() {
  return SD.begin(chipSelect_);
}

bool SdDataStore::loadLastRoutine(std::vector<StepData>& steps) {
  String name;
  if (!readSelectedRoutineName(name) || name.length() == 0) {
    return false;
  }
  return loadRoutine(name, steps);
}

bool SdDataStore::loadRoutine(const String& name, std::vector<StepData>& steps) {
  steps.clear();
  File file = SD.open("/" + name);
  if (!file) {
    return false;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    StepData step;
    const int i1 = line.indexOf(' ');
    if (i1 < 0) continue;
    step.x = line.substring(0, i1).toInt();

    line.remove(0, i1 + 1);
    const int i2 = line.indexOf(' ');
    if (i2 < 0) continue;
    step.y = line.substring(0, i2).toInt();

    line.remove(0, i2 + 1);
    const int i3 = line.indexOf(' ');
    if (i3 < 0) continue;
    step.minutes = line.substring(0, i3).toInt();
    line.remove(0, i3 + 1);
    step.seconds = line.toInt();

    steps.push_back(step);
  }

  file.close();
  writeSelectedRoutineName(name);
  return !steps.empty();
}

bool SdDataStore::saveRoutine(const std::vector<StepData>& steps, String& outFileName) {
  outFileName = nowFileName();
  File file = SD.open("/" + outFileName, FILE_WRITE);
  if (!file) {
    return false;
  }

  for (const StepData& step : steps) {
    file.print(step.x);
    file.print(' ');
    file.print(step.y);
    file.print(' ');
    file.print(step.minutes);
    file.print(' ');
    file.println(step.seconds);
  }
  file.close();

  return writeSelectedRoutineName(outFileName);
}



bool SdDataStore::listRoutineNames(std::vector<String>& names) {
  names.clear();
  File root = SD.open("/");
  if (!root) return false;

  while (true) {
    File entry = root.openNextFile();
    if (!entry) break;
    String fileName = String(entry.name());
    if (fileName.startsWith("/")) fileName.remove(0, 1);

    if (!entry.isDirectory() && fileName.endsWith(".txt") && fileName != String(positionFile_).substring(1) &&
        fileName != String(selectedFile_).substring(1)) {
      names.push_back(fileName);
    }
    entry.close();
  }
  root.close();
  return !names.empty();
}

bool SdDataStore::loadMotorPosition(int& xSteps, int& ySteps) {
  xSteps = 0;
  ySteps = 0;
  File file = SD.open(positionFile_);
  if (!file) return false;

  xSteps = file.parseInt();
  ySteps = file.parseInt();
  file.close();
  return true;
}

bool SdDataStore::saveMotorPosition(int xSteps, int ySteps) {
  File file = SD.open(positionFile_, FILE_WRITE);
  if (!file) return false;

  file.seek(0);
  file.println(xSteps);
  file.println(ySteps);
  file.close();
  return true;
}

String SdDataStore::nowFileName() const {
  DateTime now = rtc_.now();
  String name;
  name += String(now.year());
  name += "-";
  if (now.month() < 10) name += "0";
  name += String(now.month());
  name += "-";
  if (now.day() < 10) name += "0";
  name += String(now.day());
  name += "_";
  if (now.hour() < 10) name += "0";
  name += String(now.hour());
  name += "-";
  if (now.minute() < 10) name += "0";
  name += String(now.minute());
  name += "-";
  if (now.second() < 10) name += "0";
  name += String(now.second());
  name += ".txt";
  return name;
}

bool SdDataStore::readSelectedRoutineName(String& name) {
  name = "";
  File file = SD.open(selectedFile_);
  if (!file) return false;
  while (file.available()) {
    name += static_cast<char>(file.read());
  }
  name.trim();
  file.close();
  return true;
}

bool SdDataStore::writeSelectedRoutineName(const String& name) {
  File file = SD.open(selectedFile_, FILE_WRITE);
  if (!file) return false;
  file.seek(0);
  file.print(name);
  file.close();
  return true;
}
