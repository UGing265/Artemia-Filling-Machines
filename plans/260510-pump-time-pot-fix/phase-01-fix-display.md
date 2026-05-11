# Fix: Pump Time Potentiometer Display Bug

## Bug Summary
LCD display shows wrong time value because `displayUpdate()` reads pot directly instead of using `readPotTime()`, causing inconsistency between displayed time and actual pump timeout.

## Root Cause
- `displayUpdate()` (line 610-611) manually maps pot value: `map(potValue, 0, 1023, 2, 20)`
- `readPotTime()` (line 224-227) does same mapping but via function call
- Both should give same result, BUT `displayUpdate()` lacks proper LCD field clearing

## Fix Required
```cpp
// Line 3: "Time:Xs" - Change FROM:
int potValue = analogRead(timePot);
int processingTime = map(potValue, 0, 1023, 2, 20);
lcd.setCursor(0, 3);
lcd.print("Time:");
lcd.print(processingTime);
lcd.print("s            ");

// TO:
lcd.setCursor(0, 3);
lcd.print("Time:");
lcd.print(readPotTime());
lcd.print("s      ");  // 6 spaces to clear old value if previously "20s"
```

## Files to Modify
- `ArtemiaCode.ino`: Lines 609-615

## Validation
1. Turn pot to minimum (2s) → LCD shows "Time:2s"
2. Turn pot to maximum (20s) → LCD shows "Time:20s"
3. Verify pump timeout matches displayed time