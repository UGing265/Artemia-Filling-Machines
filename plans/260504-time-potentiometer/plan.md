# Plan: Time Potentiometer on A2

## Overview
Add a potentiometer on **A2** of CNC Shield V3 to control pump processing time from **2s to 20s**. Time is captured when pumping starts — **locked until pumping completes**.

## Changes (1 file)

### ArtemiaCode/ArtemiaCode.ino

**1. Add pin constant** (after line 66):
```cpp
const int timePot = A2;  // Potentiometer for pump process time (2-20s)
```

**2. Add `readPotTime()` function** (after line 220):
```cpp
int readPotTime() {
  int potValue = analogRead(timePot);
  return map(potValue, 0, 1023, 2, 20);  // 2s to 20s
}
```

**3. Add captured time variable** (near pump state variables, around line 45):
```cpp
int capturedProcessTime = 0;  // Locked time when pumping starts
```

**4. Update PUMP_FILL logic** — capture time when pump starts, use captured value:
```cpp
// When entering PUMP_FILL (around line 313-314):
capturedProcessTime = readPotTime();  // Lock in the time

// When checking timeout (line 356):
} else if (millis() - stateStartTime >= capturedProcessTime * 1000UL) {
```

**5. Update displayUpdate()** (line 592):
```cpp
// OLD:
int potValue = analogRead(speedPot);

// NEW:
int potValue = analogRead(timePot);
```

**6. Update docs/operation-rules.md** pin mapping:
- A1: Speed potentiometer (pump speed 0-255)
- A2: Time potentiometer (processing time 2-20s) ← NEW

## Behavior Rule
- **Time is locked once pumping starts** — even if user turns the pot mid-pump, the current pump cycle keeps its original time
- Time is re-read from pot at the start of each new PUMP_FILL cycle

## Dependencies
- None — single file, straightforward addition

## Validation
- [ ] A2 pin defined for time potentiometer
- [ ] `readPotTime()` returns 2-20s range
- [ ] LCD line 3 reads from A2 (timePot)
- [ ] Pump timeout uses captured/process time, not live pot value
