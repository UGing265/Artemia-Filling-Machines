# Calibration Mode Instructions

**When to calibrate:**
- First time using the pump
- Pump performance changes (low battery, different tubing, etc.)
- Flow rate seems inaccurate

---

## Step-by-Step

### 1. Prepare
Turn potentiometer to **MIN (0)** — this prevents accidental pump start.

### 2. Enter Calibration Mode
Press and **HOLD reset button for 3+ seconds**.
- After 3 seconds, LCD shows `CALIBRATING...`
- Release the button.

### 3. Pump Runs Automatically
Wait **5 seconds**.
- Pump runs at full speed automatically
- LCD shows countdown: `Time: 1s`, `Time: 2s`, etc.

### 4. Collect Output
Collect the pump output in a **measuring container**.

### 5. Input Your Measurement
After 5 seconds:
- Pump stops
- LCD shows `ENTER ML: 0.0`
- LCD shows `RATE: ---`

**Read your measuring container** — if you collected 4.2ml:
- Turn potentiometer until LCD shows `ENTER ML: 4.2`
- The RATE line updates automatically (e.g., `RATE: 0.840 ml/s`)

### 6. Save
Press reset button **briefly (short press)**.
- LCD shows `CALIB SAVED!`
- New flow rate saved to EEPROM memory

---

## Cancel Calibration

Press and **HOLD reset button for 3+ seconds** → LCD shows `CALIB CANCELLED`

---

## Verify

On next startup, LCD briefly shows your saved rate:
```
LCD OK Rate: 0.840
```

---

## How It Works

| Step | What Happens |
|------|--------------|
| Calibration runs | 5 seconds at full speed (PWM 255) |
| You measure | Actual volume collected |
| You input | Potentiometer sets measured volume (0.0 - 5.0 ml) |
| Save | Calculates `flowRate = volume / 5 seconds` |
| Auto-stop uses | `flowRate` for volume estimate |

**Example:** If 4.2ml collected in 5 seconds → `flowRate = 0.84 ml/s`
- Next time auto-stop triggers at 5ml, it runs for ~6 seconds

---

## Tips

- **Maximum input is 5.0ml** (matches TARGET_VOLUME)
- If you got more than 5ml, something is wrong with pump/tubing
- If you got less than 1ml, check for clogs
- Recalibrate if pump performance changes
