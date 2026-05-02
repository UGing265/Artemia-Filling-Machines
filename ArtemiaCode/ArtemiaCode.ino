// --- ĐỊNH NGHĨA CHÂN CHO TRỤC Z ---
const int stepPin = 2;    // Chân phát xung cho trục X
const int dirPin = 5;     // Chân điều khiển hướng cho trục X

// --- LCD 16x4 I2C (PCF8574) ---
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
LiquidCrystal_I2C lcd(0x27, 16, 4);

// --- STATE MACHINE ---
enum SystemState { IDLE, PUMPING, CALIBRATING, CALIB_INPUT };
SystemState systemState = IDLE;

// --- CALIBRATION CONSTANTS ---
const int CALIB_DURATION = 5000;     // 5 seconds
const int CALIB_LONG_PRESS = 3000;   // 3 seconds to enter calib
const int CALIB_SAVE_ADDR = 0;
const float EEPROM_DEFAULT_RATE = 0.5;
const char EEPROM_MAGIC = 'A';

// --- CALIBRATION VARIABLES ---
unsigned long calibStartTime = 0;
float calibInputVolume = 0.0;
float flowRateCalibrated = 0.5;      // Loaded from EEPROM, used for calculations

// --- PUMP STATE ---
unsigned long pumpStartTime = 0;
bool pumpRunning = false;


// --- ĐỊNH NGHĨA CHÂN CHO BƠM NHU ĐỘNG (L298N) ---
const int pumpPWM = 11;   // Chân ENA của L298N nối vào Z+ trên Shield (D11)
const int pumpDir = 13;   // Chân IN1 của L298N nối vào SpnDir trên Shield (D13)
// Lưu ý: Chân IN2 của L298N nối trực tiếp vào GND

// --- CẤU HÌNH THÔNG SỐ MOTOR ---
// Khi chỉ cắm Jumper M0 -> Chế độ 1/2 Step -> 400 bước/vòng
const int stepsPerRev = 100; // 800 step con 200 step là 90 độ

// --- LCD TRACKING VARIABLES ---
unsigned long motorSteps = 0;     // Total step pulses
unsigned long pumpRuntime = 0;    // Pump running time in ms
float pumpVolume = 0.0;          // Estimated volume in ml
int currentSpeed = 0;             // Current PWM value
unsigned long lastDisplayUpdate = 0;
const float FLOW_RATE_FALLBACK = 0.5;  // Default if no EEPROM calibration

// --- POTENTIOMETER ---
const int speedPot = A1;          // Potentiometer for pump speed control

// --- PHOTOELECTRIC SENSOR PNP + TUBE COUNTER ---
const int sensorPin = 10;         // Y+ on CNC shield (D10)
const int resetBtnPin = A0;       // Abort signal (A0)
int tubeCount = 0;
unsigned long lastSensorTrigger = 0;
const int DEBOUNCE_SENSOR = 50;
const int DEBOUNCE_BUTTON = 100;
bool lastAbortBtnState = HIGH;   // For edge detection on latch button
unsigned long calibPressStart = 0;  // For calibration long-press detection
bool calibBtnWasHigh = true;        // Previous button state for calibration

// --- AUTO-STOP PUMP ---
const float TARGET_VOLUME = 5.0;

void loadFlowRate() {
  if (EEPROM.read(CALIB_SAVE_ADDR) == EEPROM_MAGIC) {
    EEPROM.get(CALIB_SAVE_ADDR + 1, flowRateCalibrated);
  } else {
    flowRateCalibrated = EEPROM_DEFAULT_RATE;
  }
}

void saveFlowRate() {
  EEPROM.write(CALIB_SAVE_ADDR, EEPROM_MAGIC);
  EEPROM.put(CALIB_SAVE_ADDR + 1, flowRateCalibrated);
}

void abortAll() {
  // Stop pump if running
  analogWrite(pumpPWM, 0);
  digitalWrite(pumpDir, LOW);

  // Reset pump state
  pumpRuntime = 0;
  pumpVolume = 0.0;
  pumpRunning = false;
  currentSpeed = 0;

  // Reset tube count
  tubeCount = 0;

  // Exit any special mode
  systemState = IDLE;

  // Show abort message
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("!! ABORTED !!");
  delay(1000);
}

void setup() {
  // Cấu hình chân cho Stepper
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);

  // Không cần pinMode chân 8 hay digitalWrite chân 8 vì đã cắm jumper EN/GND cứng trên mạch
  // pinMode(8, OUTPUT);
  // digitalWrite(8, LOW);

  // Cấu hình chân cho Bơm (L298N)
  pinMode(pumpPWM, OUTPUT);
  pinMode(pumpDir, OUTPUT);
  digitalWrite(pumpDir, LOW);
  analogWrite(pumpPWM, 0);

  // Load calibrated flow rate from EEPROM
  loadFlowRate();

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("LCD OK");
  lcd.setCursor(8, 0);
  lcd.print("Rate:");
  lcd.print(flowRateCalibrated);
  delay(1500);

  // Potentiometer
  pinMode(speedPot, INPUT);

  // Photoelectric sensor + reset button
  pinMode(sensorPin, INPUT);
  pinMode(resetBtnPin, INPUT_PULLUP);
}

int readPotSpeed() {
  int potValue = analogRead(speedPot);
  return map(potValue, 0, 1023, 0, 255);
}

void loop() {
  // --- SENSOR DETECTION ---
  if (digitalRead(sensorPin) == HIGH &&
      millis() - lastSensorTrigger > DEBOUNCE_SENSOR) {
    tubeCount++;
    lastSensorTrigger = millis();
  }

  // --- ABORT/RESET BUTTON ---
  bool currentBtnState = digitalRead(resetBtnPin);
  if (currentBtnState == LOW && lastAbortBtnState == HIGH) {
    delay(DEBOUNCE_BUTTON);
    if (digitalRead(resetBtnPin) == LOW) {
      abortAll();
      lastAbortBtnState = LOW;
    }
  }
  if (currentBtnState == HIGH && lastAbortBtnState == LOW) {
    lastAbortBtnState = HIGH;
  }

  // --- CALIBRATION CHECK (long-press 3s) ---
  if (systemState == IDLE) {
    bool curBtn = digitalRead(resetBtnPin);

    // Button just pressed - start tracking
    if (curBtn == LOW && calibBtnWasHigh) {
      calibPressStart = millis();
    }

    // Button just released - check if long enough
    if (curBtn == HIGH && !calibBtnWasHigh) {
      if (calibPressStart > 0 && millis() - calibPressStart >= CALIB_LONG_PRESS) {
        systemState = CALIBRATING;
        calibStartTime = millis();
        lcd.clear();
      }
      calibPressStart = 0;  // Clear press tracker
    }

    calibBtnWasHigh = (curBtn == HIGH);
  }

  // --- HANDLE STATES ---
  if (systemState == CALIBRATING || systemState == CALIB_INPUT) {
    handleStateMachine();
    return;
  }

  // --- IDLE: Run stepper, then pump ---
  if (systemState == IDLE) {
    // Run stepper 1 revolution
    currentSpeed = 255;
    digitalWrite(dirPin, HIGH);
    for(int x = 0; x < stepsPerRev; x++) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(3000);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(100);
      motorSteps++;
    }

    delay(1000); // Rest 1 second
    currentSpeed = 0;

    // Now run pump for 2 seconds (auto-stop at TARGET_VOLUME)
    runPumpCycle();

    // Update LCD
    displayUpdate();
  }
}

void runPumpCycle() {
  int pumpSpeed = readPotSpeed();
  currentSpeed = pumpSpeed;
  pumpRunning = true;
  pumpStartTime = millis();
  digitalWrite(pumpDir, HIGH);
  analogWrite(pumpPWM, pumpSpeed);
  displayUpdate();

  unsigned long pumpLoopStart = millis();
  while (millis() - pumpLoopStart < 2000 && pumpRunning) {
    // Check abort
    bool btnState = digitalRead(resetBtnPin);
    if (btnState == LOW && lastAbortBtnState == HIGH) {
      delay(DEBOUNCE_BUTTON);
      if (digitalRead(resetBtnPin) == LOW) {
        analogWrite(pumpPWM, 0);
        digitalWrite(pumpDir, LOW);
        pumpRuntime = 0;
        pumpVolume = 0.0;
        pumpRunning = false;
        currentSpeed = 0;
        systemState = IDLE;
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("!! ABORTED !!");
        delay(1000);
        lastAbortBtnState = LOW;
        return;
      }
    }
    if (btnState == HIGH && lastAbortBtnState == LOW) {
      lastAbortBtnState = HIGH;
    }

    // Check volume auto-stop
    float currentVolume = (millis() - pumpStartTime) / 1000.0 * flowRateCalibrated;
    if (currentVolume >= TARGET_VOLUME) {
      analogWrite(pumpPWM, 0);
      digitalWrite(pumpDir, LOW);
      pumpRuntime += millis() - pumpStartTime;
      pumpVolume = pumpRuntime / 1000.0 * flowRateCalibrated;
      pumpRunning = false;
      currentSpeed = 0;
      displayUpdate();
      return;
    }

    delay(50);
  }

  // Normal stop after 2 seconds
  analogWrite(pumpPWM, 0);
  digitalWrite(pumpDir, LOW);
  pumpRuntime += 2000;
  pumpVolume = pumpRuntime / 1000.0 * flowRateCalibrated;
  pumpRunning = false;
  currentSpeed = 0;
  displayUpdate();
}

void handleStateMachine() {
  // --- CALIBRATING: Pump runs for 5 seconds ---
  if (systemState == CALIBRATING) {
    // Display calibration status
    lcd.setCursor(0, 0);
    lcd.print("CALIBRATING...    ");
    lcd.setCursor(0, 1);
    lcd.print("Time:");
    lcd.print((millis() - calibStartTime) / 1000);
    lcd.print("s       ");

    // Run pump at full speed
    digitalWrite(pumpDir, HIGH);
    analogWrite(pumpPWM, 255);
    currentSpeed = 255;

    // Check if 5 seconds elapsed
    if (millis() - calibStartTime >= CALIB_DURATION) {
      analogWrite(pumpPWM, 0);
      digitalWrite(pumpDir, LOW);
      systemState = CALIB_INPUT;
      calibInputVolume = 0.0;
      lcd.clear();
    }

    // Check abort during calibration (edge-triggered for latch button)
    bool currentBtnState = digitalRead(resetBtnPin);
    if (currentBtnState == LOW && lastAbortBtnState == HIGH) {
      delay(DEBOUNCE_BUTTON);
      if (digitalRead(resetBtnPin) == LOW) {
        analogWrite(pumpPWM, 0);
        digitalWrite(pumpDir, LOW);
        systemState = IDLE;
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("CALIB CANCELLED");
        delay(1000);
        lastAbortBtnState = LOW;  // Will update on release
        return;
      }
    }
    // Update on button release
    if (currentBtnState == HIGH && lastAbortBtnState == LOW) {
      lastAbortBtnState = HIGH;
    }
    return;
  }

  // --- CALIB_INPUT: User turns pot to set measured volume ---
  if (systemState == CALIB_INPUT) {
    // Read potentiometer for volume input (0.00 to 5.00 ml)
    calibInputVolume = map(analogRead(speedPot), 0, 1023, 0, 500) / 100.0;

    float newRate = calibInputVolume / (CALIB_DURATION / 1000.0);

    lcd.setCursor(0, 0);
    lcd.print("ENTER ML:");
    lcd.print(calibInputVolume, 1);
    lcd.print("    ");

    lcd.setCursor(0, 1);
    lcd.print("RATE:");
    if (newRate > 0) {
      lcd.print(newRate, 3);
    } else {
      lcd.print("---");
    }
    lcd.print(" ml/s     ");

    lcd.setCursor(0, 3);
    lcd.print("SHORT=SAVE LONG=CNL");

    // Check for button press (edge-triggered for latch button)
    bool calBtnState = digitalRead(resetBtnPin);
    if (calBtnState == LOW && lastAbortBtnState == HIGH) {
      delay(DEBOUNCE_BUTTON);
      if (digitalRead(resetBtnPin) == LOW) {
        unsigned long pressStart = millis();
        while (digitalRead(resetBtnPin) == LOW) {
          if (millis() - pressStart > CALIB_LONG_PRESS) {
            // Long press - cancel
            systemState = IDLE;
            lcd.clear();
            lcd.setCursor(0, 1);
            lcd.print("CALIB CANCELLED");
            delay(1000);
            lastAbortBtnState = LOW;  // Will update on release
            return;
          }
        }
        // Short press - save
        if (newRate > 0) {
          flowRateCalibrated = newRate;
          saveFlowRate();
        }
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("CALIB SAVED!");
        delay(1000);
        systemState = IDLE;
        lastAbortBtnState = LOW;  // Will update on release
        return;
      }
    }
    // Update on button release (LOW to HIGH) to enable next edge detect
    if (calBtnState == HIGH && lastAbortBtnState == LOW) {
      lastAbortBtnState = HIGH;
    }
    return;
  }

  // PUMPING state is now handled by runPumpCycle() in loop()
}

void displayUpdate() {
  lcd.setCursor(0, 0);
  lcd.print("Speed:");
  lcd.print(currentSpeed);
  lcd.print("    "); // Clear remaining chars

  lcd.setCursor(0, 1);
  lcd.print("Motor:");
  lcd.print(motorSteps);
  lcd.print("    ");

  lcd.setCursor(0, 2);
  lcd.print("PumpVol:");
  lcd.print(pumpVolume);
  lcd.print(" ml ");

  lcd.setCursor(0, 3);
  lcd.print("Tubes:");
  lcd.print(tubeCount);
  lcd.print("    ");
}