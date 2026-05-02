---
title: "Phase 1: Calibration Mode with EEPROM"
description: "Implement pump calibration mode with persistent FLOW_RATE storage"
status: pending
owner: claude
priority: P2
created: 2026-05-02
---

## Context Links

- Parent: [plan.md](../plan.md)
- Deps: [phase-02-abort-fix.md](./phase-02-abort-fix.md)
- Code: `ArtemiaCode/ArtemiaCode.ino`

## Overview

Add calibration mode that lets user measure actual flow rate and save to EEPROM.

### Calibration Flow

1. In IDLE, long-press reset button (>3s) to enter calibration
2. Pump runs for 5 seconds at full speed
3. After 5s, display prompts user to enter measured volume via potentiometer
4. Short-press saves new FLOW_RATE to EEPROM, long-press cancels

## Implementation Steps

### 1. Add EEPROM and State Variables

```cpp
#include <EEPROM.h>

enum SystemState { IDLE, PUMPING, CALIBRATING, CALIB_INPUT };
SystemState systemState = IDLE;

const int CALIB_DURATION = 5000;  // 5 seconds
const int CALIB_LONG_PRESS = 3000; // 3 seconds to enter calib
const int CALIB_SAVE_ADDR = 0;
const float EEPROM_DEFAULT_RATE = 0.5;

unsigned long calibStartTime = 0;
float calibInputVolume = 0.0;
float flowRateCalibrated = 0.5;
```

### 2. Load FLOW_RATE from EEPROM at startup

```cpp
void loadFlowRate() {
  if (EEPROM.read(CALIB_SAVE_ADDR) == 'A') {
    EEPROM.get(CALIB_SAVE_ADDR + 1, flowRateCalibrated);
  } else {
    flowRateCalibrated = EEPROM_DEFAULT_RATE;
  }
}
```

### 3. Save FLOW_RATE to EEPROM

```cpp
void saveFlowRate() {
  EEPROM.write(CALIB_SAVE_ADDR, 'A');
  EEPROM.put(CALIB_SAVE_ADDR + 1, flowRateCalibrated);
}
```

### 4. Add long-press detection for entering calibration

In `loop()` during IDLE, check long-press:

```cpp
// In IDLE loop:
if (digitalRead(resetBtnPin) == LOW) {
  unsigned long pressStart = millis();
  while (digitalRead(resetBtnPin) == LOW) {
    if (millis() - pressStart > CALIB_LONG_PRESS) {
      // Enter calibration
      systemState = CALIBRATING;
      calibStartTime = millis();
      digitalWrite(pumpDir, HIGH);
      analogWrite(pumpPWM, 255);  // Full speed
      return;
    }
  }
}
```

### 5. Handle CALIBRATING state

```cpp
if (systemState == CALIBRATING) {
  displayUpdate(); // Show calibrating status
  if (millis() - calibStartTime >= CALIB_DURATION) {
    analogWrite(pumpPWM, 0);
    systemState = CALIB_INPUT;
  }
  return; // Skip pump loop
}
```

### 6. Handle CALIB_INPUT state

```cpp
if (systemState == CALIB_INPUT) {
  // Read potentiometer for volume input
  calibInputVolume = map(analogRead(speedPot), 0, 1023, 0, 500) / 100.0; // 0.00 to 5.00 ml

  lcd.setCursor(0, 0);
  lcd.print("ENTER ML:");
  lcd.print(calibInputVolume);

  lcd.setCursor(0, 1);
  lcd.print("RATE:");
  float newRate = calibInputVolume / (CALIB_DURATION / 1000.0);
  lcd.print(newRate);

  lcd.setCursor(0, 3);
  lcd.print("BTN=SAVE LONG=CANCEL");
  return;
}
```

### 7. Short-press in CALIB_INPUT saves, long-press cancels

```cpp
// After calibration input:
if (digitalRead(resetBtnPin) == LOW) {
  unsigned long pressStart = millis();
  while (digitalRead(resetBtnPin) == LOW) {
    if (millis() - pressStart > CALIB_LONG_PRESS) {
      systemState = IDLE;
      lcd.setCursor(0, 2);
      lcd.print("CALIB CANCELLED   ");
      return;
    }
  }
  // Short press - save
  flowRateCalibrated = calibInputVolume / (CALIB_DURATION / 1000.0);
  saveFlowRate();
  systemState = IDLE;
}
```

### 8. Use flowRateCalibrated instead of FLOW_RATE constant

Replace `FLOW_RATE` usage with `flowRateCalibrated` in volume calculations.

## Todo List

- [ ] Add EEPROM library and state variables
- [ ] Implement loadFlowRate() on startup
- [ ] Implement saveFlowRate() function
- [ ] Add long-press detection to enter calibration
- [ ] Handle CALIBRATING state (5s pump run)
- [ ] Handle CALIB_INPUT state (potentiometer volume input)
- [ ] Implement short-press save, long-press cancel
- [ ] Replace FLOW_RATE with flowRateCalibrated

## Success Criteria

- Long-press 3s enters calibration mode
- Pump runs exactly 5 seconds during calibration
- Potentiometer allows setting 0-5ml measured volume
- Short-press saves new FLOW_RATE to EEPROM
- Long-press cancels without saving
- FLOW_RATE persists across reboots
- Existing pump/lcd code unchanged (except uses new flow rate)

## Risk Assessment

- EEPROM wear: minimal (only on user-initiated calibration)
- Accuracy: depends on user measurement precision
- Debounce: long- press detection needs stable reading
