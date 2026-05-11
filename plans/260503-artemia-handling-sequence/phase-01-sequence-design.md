# Phase 1: Artemia Handling Sequence Design

## Context Links
- Main code: `ArtemiaCode/ArtemiaCode.ino`
- Plan: `plans/260503-artemia-handling-sequence/plan.md`

## Overview
- Priority: P1 (High)
- Status: Completed
- Brief description: Design sensor-triggered pump sequence replacing current hardcoded rotation+pump cycle

## Current Behavior (loop())

Current code in `loop()`:
1. Run stepper 1 revolution (stepsPerRev steps)
2. delay(1000) - rest 1 second
3. runPumpCycle() - pump 2 seconds OR until 5ml
4. displayUpdate()

This is HARDCODED - no sensor awareness.

## Required Behavior

```
loop()
├── Check abort button (existing)
├── Handle calibration states (existing)
└── IDLE state → NEW SEQUENCE:

    Step A: Rotate 1 revolution
    │
    Step B: Check sensor immediately after rotation
    │       └── If sensor HIGH (tube detected):
    │               ├── Wait 3s
    │               ├── Pump ON → auto-stop at 5ml (timeout 5s)
    │               ├── Pump OFF
    │               ├── tubeCount++
    │               ├── Wait 5s
    │               └── → back to Step A
    │       └── If sensor LOW (no tube):
    │               ├── Wait 10s (user places tube)
    │               └── → back to Step A
    │
```

## Key Design Decisions

### 1. Sensor Detection Timing
- Sensor is checked AFTER rotation completes
- If tube detected → 3s delay → pump (5ml over ~5s)
- If no tube → 10s wait → next rotation (giving user time to place tube)

### 2. 3s Delay Before Pumping
- Purpose: Let tube settle into position
- Prevents premature pumping

### 3. Pump Auto-Stop
- Stop at TARGET_VOLUME (5ml) with ~5s timeout
- Use flowRateCalibrated for volume estimation

### 4. Post-Pump Sequence
- Increment tubeCount
- Wait 5s (tube settling)
- Continue to next rotation

### 5. No-Tube Timeout
- After full rotation with no tube detected
- Wait 10s (polling sensor during this time)
- If tube placed within 10s → trigger pump sequence
- If timeout → continue rotation anyway

## Sequence State Machine

```cpp
enum HandleState {
  ROTATING,        // Stepper rotating 1 revolution
  SENSOR_CHECK,   // After rotation, check sensor
  PUMP_WAIT,      // Waiting 1s before pump
  PUMPING,        // Pump running, auto-stop
  PUMP_DONE,      // Pump stopped, wait 2s
  NO_TUBE_WAIT    // No tube detected, wait 10s
};
```

## Edge Cases

| Case | Handling |
|------|----------|
| Tube removed during pumping | Continue until 5ml |
| Sensor flickers | Debounce 50ms (existing DEBOUNCE_SENSOR) |
| Button pressed during sequence | abortAll() immediately |
| Power loss during pump | pumpRunning flag reset on startup |

## LCD Display Updates

During sequence:
```
Line 0: "MAY CHIET ARTERMIA"
Line 1: "Speed:XXX"
Line 2: "Tubes:X |....|"
Line 3: "Time:Xs"
```

Where:
- Speed: current pump PWM (0-255)
- Tubes: count of filled tubes
- Time: processing time (adjustable 2s-20s via potentiometer)
- Bar: filled `|` = tube detected, empty `-` = no tube

Where YYY = ROTATE | WAIT1S | PUMPING | WAIT2S | NO_TUBE | DONE

## Related Code Changes

Files to modify: `ArtemiaCode/ArtemiaCode.ino`
- Add `HandleState` enum
- Add state variable `HandleState handleState = ROTATING`
- Add state timer `unsigned long stateStartTime = 0`
- Add processing time variable `int processingTimeS = 5` (default, adjustable 2-20s)
- Replace IDLE loop content with state machine
- Rewrite `displayUpdate()` to new LCD layout (MAY CHIET ARTERMIA title)
- Add sensor status to LCD (HI/LO)
- Potentiometer A1 adjusts processing time (2s-20s mapped from 0-1023)

## Implementation Steps

1. Add `HandleState` enum after existing enums
2. Add state variables: `handleState`, `stateStartTime`
3. Rewrite IDLE loop section as state machine
4. Add `checkOverflow()` helper in pump cycle
5. Add LCD state display in `displayUpdate()`
6. Test sequence with physical hardware

## Todo List
- [ ] Add HandleState enum
- [ ] Add state variables
- [ ] Implement state machine in loop()
- [ ] Update displayUpdate() with state info
- [ ] Compile and verify

## Success Criteria
- [ ] Sensor HIGH → 3s delay → pump fills 5ml → stops
- [ ] After pump: tubeCount++, wait 5s, rotate
- [ ] No tube after rotation → wait 10s, rotate (no pump)
- [ ] Processing time adjustable via pot (2s-20s)
- [ ] LCD shows: MAY CHIET ARTERMIA, Speed, Tubes, Time, Sensor status
- [ ] Abort button works during any state

## Risk Assessment
- Risk: State machine complexity
- Mitigation: Keep states simple, each does one thing
- Risk: Timing sensitive (1s, 2s, 10s)
- Mitigation: Use millis() for accurate timing

## Security Considerations
- User can always abort via button
- Overflow protection prevents mess
- No network/internet access