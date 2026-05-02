---
title: "LCD Display Logic Phase"
description: "Add tracking variables and display update logic for speed, motor rotation, pump volume"
status: completed
priority: P2
effort: 1h
branch: main
tags: [arduino, lcd]
created: 2026-05-02
---

# Phase 02: LCD Display Logic

## Context Links

- Phase 01: `./phase-01-lcd-init.md`
- Original code: `ArtemiaCode/ArtemiaCode.ino`

## Overview

- Priority: P2
- Status: Pending
- Add tracking variables and display update logic

## Key Insights

- Motor rotation counted via step pulses (each step = 1.8° for 200-step motor, or 0.9° for 400-step)
- Pump volume estimated: runtime_seconds × flow_rate_constant (ml/s)
- Update LCD every loop iteration, but throttle updates to avoid flicker

## Requirements

### Variables to Add

```cpp
// Tracking variables
unsigned long motorSteps = 0;      // Total step pulses
unsigned long pumpRuntime = 0;    // Pump running time in ms
float pumpVolume = 0.0;           // Estimated volume in ml
int currentSpeed = 0;              // Current PWM value
bool pumpRunning = false;          // Pump state flag
unsigned long pumpStartTime = 0;  // For runtime tracking
```

### Display Format (16x4)

| Line | Content | Example |
|------|---------|---------|
| 1 | Speed: XXX | Speed: 255 |
| 2 | Motor: XXXX | Motor: 1234 |
| 3 | PumpVol: XXXX ml | PumpVol: 45.2 ml |
| 4 | (spare) | |

### Display Update Logic

- Update LCD every ~500ms (not every loop)
- Use `millis()` for non-blocking timing
- Print values with `lcd.print()`

## Related Code Files

- Modify: `ArtemiaCode/ArtemiaCode.ino`

## Implementation Steps

1. Add tracking variables after pump pin definitions
2. In loop(), after `analogWrite(pumpPWM, 255)`:
   - Set pumpRunning = true, pumpStartTime = millis()
3. After `analogWrite(pumpPWM, 0)`:
   - pumpRuntime += millis() - pumpStartTime
   - pumpVolume = pumpRuntime / 1000.0 * FLOW_RATE (ml/s)
   - pumpRunning = false
4. After step pulse in loop():
   - motorSteps++
5. Add displayUpdate() function called every 500ms
6. displayUpdate() shows Speed, Motor count, PumpVol on LCD

## Todo List

- [ ] Add tracking variables
- [ ] Track pump runtime
- [ ] Track motor steps
- [ ] Calculate pump volume
- [ ] Add displayUpdate() function
- [ ] Call displayUpdate() in loop

## Success Criteria

- LCD shows Speed, Motor count, PumpVol updated in real-time
- Values update correctly when motor/pump run

## Risk Assessment

- Division by zero in volume calc (guard with if pumpRuntime > 0)
- Integer overflow for long runtime (use unsigned long)

## Security Considerations

- None