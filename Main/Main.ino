#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "RobotTypes.h"
#include "LcdDisplay.h"
#include "EncoderInput.h"
#include "SdDataStore.h"
#include "MotionController.h"
#include "RoutineCoordinator.h"

namespace {
constexpr int CHIP_SELECT = 5;

constexpr int CLK_A = 33;
constexpr int DT_A = 32;
constexpr int BUTTON_A = 39;
constexpr int CLK_B = 35;
constexpr int DT_B = 34;
constexpr int BUTTON_B = 36;

constexpr int DIRX = 26;
constexpr int PULX = 25;
constexpr int DIRY = 14;
constexpr int PULY = 27;
constexpr int MOTOR_SPEED_US = 300;
}

LiquidCrystal_I2C lcdDriver(0x27, 2, 1, 0, 4, 5, 6, 7);
RTC_DS3231 rtc;

LcdDisplay lcd(lcdDriver);
EncoderInput encoder(CLK_A, DT_A, BUTTON_A, CLK_B, DT_B, BUTTON_B);
SdDataStore dataStore(CHIP_SELECT, rtc);
MotionController motor(DIRX, PULX, DIRY, PULY, MOTOR_SPEED_US);
RoutineCoordinator coordinator(lcd, encoder, dataStore, motor, rtc);

void setup() {
  Serial.begin(115200);

  lcd.begin();
  encoder.begin();
  motor.begin();

  if (!rtc.begin()) {
    lcd.showError("RTC error");
    while (true) {}
  }

  if (!dataStore.begin()) {
    lcd.showError("SD init error");
    while (true) {}
  }

  coordinator.begin();
}

void loop() {
  coordinator.update();
}
