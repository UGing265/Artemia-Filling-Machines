---
title: "Photoelectric Sensor PNP Fix"
description: "Fix sensor logic for PNP type sensor - detect HIGH instead of LOW"
status: completed
priority: P1
effort: 15m
branch: main
tags: [arduino, debug, photoelectric-sensor, bugfix]
created: 2026-05-02
---

# Photoelectric Sensor PNP Fix Plan

## Root Cause CONFIRMED

**Sensor is PNP type.** PNP sensors output HIGH when object detected (opposite of NPN).

| Sensor Type | Output When Object DETECTED | Code Check |
|-------------|----------------------------|------------|
| NPN (open-collector) | LOW | `digitalRead() == LOW` |
| **PNP (active-high)** | **HIGH** | `digitalRead() == HIGH` |

**Current code is wrong:**
```cpp
if (digitalRead(sensorPin) == LOW &&  // ← Wrong for PNP!
```

This causes random noise LOW signals to trigger count++, making it auto-count.

## Fix (Single Change)

### In ArtemiaCode.ino, change line ~78:

**FROM:**
```cpp
  if (digitalRead(sensorPin) == LOW &&
      millis() - lastSensorTrigger > DEBOUNCE_SENSOR) {
```

**TO:**
```cpp
  if (digitalRead(sensorPin) == HIGH &&
      millis() - lastSensorTrigger > DEBOUNCE_SENSOR) {
```

### Also change pinMode from INPUT_PULLUP to INPUT:

**FROM:**
```cpp
  pinMode(sensorPin, INPUT_PULLUP);
```

**TO:**
```cpp
  pinMode(sensorPin, INPUT);
```

**Reason:** PNP outputs HIGH when detected, so no pullup needed. The sensor's own output drives the line. A 10kΩ pull-down resistor between signal and GND is recommended to prevent floating when sensor is OFF.

## Hardware Addition (Optional)

Add 10kΩ pull-down resistor:
```
Signal (sensor) ─────┬──── Arduino pin D10
                     │
                   10kΩ
                     │
                    GND
```

This prevents floating noise when sensor output is HIGH-to-HIGHZ (transition state).

## Todo List

- [ ] Change `digitalRead(sensorPin) == LOW` to `HIGH`
- [ ] Change `pinMode(sensorPin, INPUT_PULLUP)` to `INPUT`
- [ ] (Optional) Add 10kΩ pull-down resistor

## Success Criteria

- Sensor shows HIGH when object detected
- tubeCount increments only when object passes
- No auto-counting when nothing is in front of sensor
- Existing pump/lcd code unchanged
