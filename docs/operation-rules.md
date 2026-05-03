# Artemia Filling Machine - Complete Operation Rules

## Hardware Configuration

### Pin Mapping
| Pin | Function |
|-----|----------|
| D2 | Stepper step pulse (X axis) |
| D5 | Stepper direction (X axis) |
| D9 | Limit switch X+ (homing) |
| D10 | Photoelectric sensor (Y+ input) |
| A1 | Speed potentiometer (pump speed 0-255) |
| A2 | Time potentiometer (pump process time 2-20s) |
| D11 | Pump PWM (L298N ENA → Z+) |

### Motor Settings
- Stepper: 100 steps/revolution (half-step mode, 400 steps/rotation, 2 rotations = homing)
- Pump: L298N driver, IN2 grounded
- Target fill volume: **5.0 ml**

---

## Startup Sequence

1. LCD shows `LCD OK Rate: X.XXX` for 1.5s
2. Homing sequence runs:
   - Rotates up to 2 full rotations toward limit switch
   - If limit switch triggered → `HOMING DONE!` → 10s countdown → start
   - If limit switch **NOT** triggered → `PLEASE TURN OFF SYSTEM !!!` (machine stops)
3. Machine enters IDLE state

---

## Main Handle Sequence (IDLE State)

```
ROTATING → SENSOR_CHECK → [tube?] → PUMP_WAIT (3s) → PUMP_FILL → PUMP_DONE (5s) → ROTATING
                                          ↓
                         [no tube] → NO_TUBE_WAIT (10s, poll) → ROTATING
```

### Step 1: ROTATING
- Stepper rotates **1 full revolution** (100 step pulses)
- Direction: HIGH (forward)
- Pulse: 3000µs on, 100µs off
- After completion → SENSOR_CHECK

### Step 2: SENSOR_CHECK
- Read photoelectric sensor (D10)
- If **HIGH** (tube detected) → PUMP_WAIT
- If **LOW** (no tube) → NO_TUBE_WAIT
- No timeout — immediate transition

### Step 3: PUMP_WAIT (3s confirmation)
- Wait up to **3 seconds** for tube to settle
- **Safety: If tube disappears during wait** → skip immediately, return to ROTATING
- After 3s elapsed → PUMP_FILL

### Step 4: PUMP_FILL
- Start pump at potentiometer-set speed (0-255 PWM)
- Auto-stop when volume reaches **5.0 ml**
- **Process time: 2-20 seconds** (set by A2 time potentiometer, captured when pumping starts)
- **Time is locked once pumping starts** — even if pot is turned mid-pump, current cycle keeps its original time
- **Safety: If tube disappears during pumping** → stop pump immediately, do NOT count, show `TUBE LOST! SKIP`, return to ROTATING
- On success → increment tubeCount → PUMP_DONE

### Step 5: PUMP_DONE
- Wait **5 seconds** post-fill
- Then return to ROTATING

### Step 6: NO_TUBE_WAIT (10s skip)
- Wait up to **10 seconds** for tube to be placed
- **If tube placed during wait** → go to PUMP_WAIT immediately
- After 10s timeout → return to ROTATING (skip)

---

## Safety Rules Summary

| Event | Action |
|-------|--------|
| Tube disappears during 3s wait | Skip, continue rotation |
| Tube disappears during pumping | Stop pump, no count, show `TUBE LOST! SKIP` |
| Abort button short press | Stop everything, reset tubeCount to 0, return to IDLE |

---

## Calibration Mode

**Enter:** Long press reset button (3+ seconds) while in IDLE

**Phase 1 - CALIBRATING (5s):**
- Pump runs at full speed (PWM 255)
- LCD shows `Time: Xs`
- User collects output
- Can abort: short press → `CALIB CANCELLED`

**Phase 2 - CALIB_INPUT:**
- LCD shows `ENTER ML: X.X` (potentiometer sets 0.0-5.0 ml)
- LCD shows `RATE: X.XXX ml/s` (calculated from volume / 5s)
- LCD shows `SHORT=SAVE LONG=CNL`

**Save:** Short press button → saves flowRate to EEPROM → `CALIB SAVED!`

**Cancel:** Long press button (3+ seconds) → `CALIB CANCELLED`

**EEPROM:** Stored at address 0, magic char 'A', float value

---

## Abort / Reset

**Trigger:** Short press reset button (any state except during calibration input)

**Actions:**
1. Stop pump immediately (PWM 0, dir LOW)
2. Reset pumpRuntime, pumpVolume, pumpRunning, currentSpeed to 0
3. Reset tubeCount to 0
4. Set systemState = IDLE
5. Set handleState = ROTATING
6. Show `!! ABORTED !!` on LCD for 1 second

---

## LCD Display (16x4 I2C)

| Line | Content | Description |
|------|---------|-------------|
| 0 | `MAY CHIET ARTERMIA` | Machine name |
| 1 | `Speed:XXX` | Current pump PWM (0-255) |
| 2 | `Tubes:X ||||||` or `Tubes:X ------` | Tube count + sensor bar (| = tube present, - = empty) |
| 3 | `Time:Xs` | Processing time from potentiometer (2-20s map) |

---

## State Machine Reference

```
SystemState: IDLE | PUMPING | CALIBRATING | CALIB_INPUT

HandleState: ROTATING | SENSOR_CHECK | PUMP_WAIT | PUMP_FILL | PUMP_DONE | NO_TUBE_WAIT
```

**During CALIBRATING or CALIB_INPUT:** handleStateMachine() runs instead of main loop handle sequence

**Note:** PUMPING state exists in enum but main pumping logic is handled inside HandleState PUMP_FILL. runPumpCycle() is a separate function (unused in normal loop flow).

---

## Constants Reference

| Constant | Value | Usage |
|----------|-------|-------|
| PUMP_WAIT_DELAY | 3000ms | 3s tube confirmation |
| POST_PUMP_DELAY | 5000ms | 5s after pump stops |
| NO_TUBE_DELAY | 10000ms | 10s skip wait |
| PUMP_TIMEOUT | 5000ms | Max pump runtime (fallback) |
| CALIB_DURATION | 5000ms | 5s calibration pump |
| CALIB_LONG_PRESS | 3000ms | Long press threshold |
| TARGET_VOLUME | 5.0 ml | Auto-stop volume |
| PROCESS_TIME_MIN | 2 s | Min pump process time |
| PROCESS_TIME_MAX | 20 s | Max pump process time |
| EEPROM_DEFAULT_RATE | 0.5 ml/s | Fallback flow rate |
| DEBOUNCE_SENSOR | 50ms | Sensor debounce |
| DEBOUNCE_BUTTON | 100ms | Button debounce |
| stepsPerRev | 100 | Stepper steps per revolution |

---

## Edge Cases

- **Potentiometer at 0:** Pump speed = 0, pump never fills, will timeout after 5s
- **First boot (no EEPROM):** Uses default flow rate 0.5 ml/s
- **Calibration input = 0:** Rate shows `---`, short press does not save
- **Calibration cancelled:** Returns to IDLE without saving
- **Homing fails:** Machine halts with `PLEASE TURN OFF SYSTEM !!!`
- **Tube removed mid-fill:** Tube is NOT counted, machine skips to next rotation