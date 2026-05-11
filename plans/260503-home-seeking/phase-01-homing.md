# Phase 1: Homing Implementation

## Context Links
- Main code: `ArtemiaCode/ArtemiaCode.ino`
- Plan: `plans/260503-home-seeking/plan.md`

## Overview
- Priority: P1 (High)
- Status: Pending
- Brief description: Implement slow 360° homing rotation on startup with Z-limit switch detection

## Hardware Pinout
CNC Shield V3 pin mapping:
| Signal | Arduino Pin | CNC Shield V3 |
|--------|-------------|---------------|
| Step (Z) | D4 | Z+ |
| Dir (Z) | D7 | Z- |
| Limit Switch (Z) | D9 | Z-Limit |

## Key Insights
- Current stepper uses stepPin=2, dirPin=5 for "X" axis
- CNC Shield V3: Z-axis step/dir on D4/D7 (not D2/D5 as in code - code may be non-standard)
- Z-limit switch on D9 (Arduino pin 9 on CNC Shield)
- User confirmed limit switch is on Z pin

## Requirements

### Functional Requirements
1. On `setup()` after LCD init, run homing sequence
2. Rotate motor SLOWLY (wider pulse width) for 2 full rotations max
3. Check Z-limit switch (D9) every step - if pressed, stop immediately
4. If home found: wait 10 seconds, LCD shows "ACTIVE", then proceed to normal loop
5. If home NOT found after 2 rotations: LCD shows "TURN OFF-ON" error, loop forever (no pump)

### Non-Functional Requirements
- Homing speed: slower than normal (safer for homing)
- Timeout: exactly 2 full rotations (~8 seconds at slow speed)
- No pump activity during homing

## Architecture

```
setup()
  └─ lcdInit()
  └─ homeSeekingSequence()    // NEW
       ├─ Set direction (toward home - user may specify)
       ├─ For 2 rotations:
       │   └─ For each step:
       │       ├─ digitalWrite(stepPin, HIGH)
       │       ├─ delayMicroseconds(3000)
       │       ├─ digitalWrite(stepPin, LOW)
       │       ├─ delayMicroseconds(500)
       │       └─ Check limit switch (D9) → if pressed, BREAK
       └─ If switch found:
           ├─ Stop motor
           ├─ LCD: "HOMING DONE"
           ├─ Wait 10s
           ├─ LCD: "ACTIVE"
           └─ Return (proceed to loop)
       └─ If timeout:
           ├─ LCD: "TURN OFF-ON"
           └─ while(1) {}  // Infinite loop - user must reset
```

## Related Code Files
- `ArtemiaCode/ArtemiaCode.ino`: Modify `setup()` to add homing sequence

## Implementation Steps

1. **Add Z-limit switch pin definition** (after existing pin definitions):
   ```cpp
   const int limitSwitchZ = 9;  // Z-limit on CNC Shield V3 (D9)
   ```

2. **Configure pin mode in `setup()`**:
   ```cpp
   pinMode(limitSwitchZ, INPUT_PULLUP);
   ```

3. **Add `homeSeekingSequence()` function before `setup()`**:
   - Use slower step speed: `delayMicroseconds(3000)` high, `delayMicroseconds(500)` low
   - Loop for 2 × `stepsPerRev` iterations
   - Check `digitalRead(limitSwitchZ) == LOW` at each step
   - If switch found: stop, display "HOMING DONE", wait 10s, display "ACTIVE", return
   - If timeout: display "TURN OFF-ON", infinite loop

4. **Call `homeSeekingSequence()` in `setup()`** after LCD init, before pump/button config

## Todo List
- [ ] Add limit switch pin definition
- [ ] Configure pin mode INPUT_PULLUP
- [ ] Implement homeSeekingSequence() function
- [ ] Call homing in setup() after LCD init
- [ ] Verify compilation

## Success Criteria
- [ ] Motor rotates slowly on startup
- [ ] If limit switch pressed within 2 rotations: motor stops, 10s wait, LCD shows "ACTIVE", pump runs
- [ ] If limit switch NOT pressed within 2 rotations: LCD shows "TURN OFF-ON", system halts
- [ ] Code compiles without errors

## Risk Assessment
- **Risk**: Motor spins continuously if switch wiring is wrong
- **Mitigation**: 2-rotation timeout prevents indefinite spinning
- **Risk**: Switch is NC (normally closed) vs NO (normally open)
- **Mitigation**: Test with `INPUT_PULLUP` and check logic - if NC, detect HIGH instead of LOW
