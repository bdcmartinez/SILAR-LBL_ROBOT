# SILAR-LBL Robot Code Guide

This document explains the code structure of the repository and how the firmware is organized.

## Repository structure

- `Main/Main.ino`: Main refactored firmware with class-based organization (`Files`, `Encoder`) and SD-based recipe handling.
- `Programa/Programa.ino`: Earlier monolithic firmware version with equivalent control flow and menu logic.
- `README.md`: High-level project and hardware overview.

## Firmware purpose

The firmware controls a thin-film deposition robot for SILAR / Layer-by-Layer processes using:

- An ESP32 controller.
- Two stepper axes (X and Y).
- Two rotary encoders + push buttons for user input.
- A DS3231 RTC for process timing.
- An I2C LCD for UI/status.
- An SD card for persisting recipes and position state.

## Main control concepts

### 1) Motion and position

Both sketches define step/dir pins for two motors:

- X axis: `DIRX` + `PULX`
- Y axis: `DIRY` + `PULY`

Encoder turns update internal position counters and convert UI increments to motor steps.

### 2) Recipe model

A process recipe is represented by parallel arrays:

- `X[i]`: X destination/index for step `i`
- `Y[i]`: Y destination/index for step `i`
- `MINUTOS[i]`: hold time minutes for step `i`
- `SEGUNDOS[i]`: hold time seconds for step `i`

`Main/Main.ino` reads/writes these arrays to files on SD so complete routines can be saved and replayed.

### 3) Timekeeping

The DS3231 RTC is used to:

- Build timestamp-based recipe filenames.
- Run each recipe step for the programmed duration.
- Display elapsed step and total process time.

### 4) Persistence on SD

The firmware persists:

- Last motor position (`/pos.txt`).
- Last selected routine name (`/fileName.txt`).
- Recipe files (timestamped names like `YYYY-MM-DD_HH-MM-SS.txt`).

## Main components in `Main/Main.ino`

### `Files` class

Responsibilities:

- Build file associations from SD root.
- Track selected recipe filename.
- Parse recipe files into process arrays.
- Save current arrays as a new timestamped recipe file.
- Save/load persistent metadata like current position.

### `Encoder` class

Responsibilities:

- Debounced handling for encoder push buttons.
- Encoder interrupt logic for rotary movement.
- UI state variables (`POS_A`, `POS_B`) and derived step counts (`STEPSX`, `STEPSY`).
- Boundary limits for menu selections and motor step ranges.

## `Programa/Programa.ino` notes

`Programa.ino` keeps the same project logic but in a single sketch-style flow:

- Global state and arrays.
- `setup()` configures pins, LCD, interrupts, RTC, and SD.
- `loop()` hosts menu navigation and process execution.

It can be used as a reference for understanding the evolution toward the class-based `Main/Main.ino` structure.

## Typical execution flow

1. Initialize LCD, pins, interrupts, RTC, and SD.
2. Load last saved state and/or selected recipe.
3. User configures steps/cycles through encoder-driven menus.
4. On start, firmware iterates through recipe steps and layers.
5. For each step: move motors, then hold for configured RTC-timed duration.
6. Save position and/or recipe updates back to SD.

## Suggested next improvements

- Add bounds checks before writing arrays to avoid overflow.
- Add recipe validation when parsing malformed lines.
- Use named constants for maximum steps (`20`, `40`) to reduce magic numbers.
- Split motor/UI/storage responsibilities into separate modules for easier testing.
