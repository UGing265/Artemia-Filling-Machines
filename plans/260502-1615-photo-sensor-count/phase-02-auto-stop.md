---
title: "Phase 2: 5ml Auto-Stop Pump + LCD Tube Count"
description: "Implement pump auto-stop at 5ml and LCD display for tube count"
status: completed
owner: claude
priority: P2
created: 2026-05-02
---

## Context Links

- Parent: [plan.md](../plan.md)
- Deps: [phase-01-sensor-counter.md](./phase-01-sensor-counter.md)
- Code: `ArtemiaCode/ArtemiaCode.ino`

## Overview

Add auto-stop pump when 5ml reached and show tube count on LCD row 3.

### Auto-Stop Logic

- Existing `pumpVolume` calculated from: `pumpRuntime / 1000.0 * FLOW_RATE`
- Add check: if `pumpVolume >= 5.0` → stop pump immediately
- In existing pump running code, add condition to turn off pumpPWM

### LCD Display (row 3)

- Show `Tubes: XX` on LCD row 3
- Clear remaining chars to avoid ghosting

## Implementation Steps

1. Add constant: `const float TARGET_VOLUME = 5.0;`
2. In pump running block:
   - After `analogWrite(pumpPWM, pumpSpeed)` add check
   - Before delay: calculate current volume
   - If volume >= 5.0: turn off pump, skip delay
3. In `displayUpdate()`:
   - Add row 3 display: `lcd.print("Tubes: ")` + tubeCount

## Code Snippet

```cpp
const float TARGET_VOLUME = 5.0;

// Inside pump running block:
analogWrite(pumpPWM, pumpSpeed);
delay(2000);

// Add auto-stop check inside the running block:
float currentVolume = (millis() - pumpStartTime) / 1000.0 * FLOW_RATE;
if (currentVolume >= TARGET_VOLUME) {
  analogWrite(pumpPWM, 0);  // Stop pump
  pumpRuntime += millis() - pumpStartTime;
  pumpVolume = pumpRuntime / 1000.0 * FLOW_RATE;
  pumpRunning = false;
  currentSpeed = 0;
  displayUpdate();
  return;  // Skip remaining delay
}

// In displayUpdate():
lcd.setCursor(0, 3);
lcd.print("Tubes:");
lcd.print(tubeCount);
lcd.print("    ");
```

## Todo List

- [ ] Add TARGET_VOLUME constant
- [ ] Add auto-stop check in pump logic
- [ ] Add Tubes: display on LCD row 3
- [ ] Verify compilation

## Success Criteria

- Pump stops automatically when volume >= 5ml
- LCD shows tube count on row 3
- Existing sensor/button logic still works
- No compilation errors