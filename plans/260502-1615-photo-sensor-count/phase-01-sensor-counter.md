---
title: "Phase 1: Photoelectric Sensor + Tube Counter"
description: "Implement photoelectric sensor detection and tube counting with reset button"
status: completed
owner: claude
priority: P2
created: 2026-05-02
---

## Context Links

- Parent: [plan.md](../plan.md)
- Code: `ArtemiaCode/ArtemiaCode.ino`

## Overview

Add photoelectric sensor to count tubes and push button to reset counter.

### Pin Assignment

| Component | Pin | Mode |
|-----------|-----|------|
| Photoelectric sensor | D10 (Y+) | INPUT_PULLUP (NC = no object, LOW = object detected) |
| Reset button | A0 (Abort) | INPUT_PULLUP (NC = not pressed, LOW = pressed) |

### Debounce

- Sensor: 50ms debounce to avoid double-counting
- Button: 100ms debounce for mechanical switch

## Requirements

1. Photoelectric sensor on D7 - detect falling edge (HIGH→LOW transition means object detected)
2. Tube counter variable `tubeCount` starts at 0
3. Each detection: tubeCount++
4. Push button on D6 - when pressed, set tubeCount = 0
5. No LCD changes in this phase

## Implementation Steps

1. Define pin constants: `sensorPin = 7`, `resetBtnPin = 6`
2. Add `tubeCount` global variable (int)
3. Add `lastSensorTrigger = 0` for debounce timing
4. In setup(): `pinMode(sensorPin, INPUT_PULLUP)`, `pinMode(resetBtnPin, INPUT_PULLUP)`
5. In loop(): implement detection logic:
   - Read sensor: if LOW and (now - lastSensorTrigger) > 50ms → count++
   - Read button: if LOW → reset tubeCount to 0
   - Update lastSensorTrigger on count

## Code Snippet

```cpp
const int sensorPin = 10;   // Y+ on CNC shield (D10)
const int resetBtnPin = A0; // Abort signal (A0)

int tubeCount = 0;
unsigned long lastSensorTrigger = 0;
const int DEBOUNCE_SENSOR = 50;
const int DEBOUNCE_BUTTON = 100;

void setup() {
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(resetBtnPin, INPUT_PULLUP);
}

void loop() {
  // Sensor detection (NC = HIGH, detected = LOW)
  if (digitalRead(sensorPin) == LOW &&
      millis() - lastSensorTrigger > DEBOUNCE_SENSOR) {
    tubeCount++;
    lastSensorTrigger = millis();
  }

  // Reset button (NC = HIGH, pressed = LOW)
  if (digitalRead(resetBtnPin) == LOW) {
    tubeCount = 0;
    delay(DEBOUNCE_BUTTON);
  }
}
```

## Todo List

- [ ] Define pin constants
- [ ] Add tubeCount and debounce variables
- [ ] Configure pin modes in setup()
- [ ] Implement detection + count logic in loop()
- [ ] Implement reset button logic

## Success Criteria

- Sensor triggers count when object passes in front of it
- Button press resets counter to 0
- No double-counting from bounce
- Existing pump/lcd code unchanged