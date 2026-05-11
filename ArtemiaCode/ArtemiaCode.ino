// --- ĐỊNH NGHĨA CHÂN CHO TRỤC X ---
const int stepPin = 2;    // Chân phát xung cho trục X
const int dirPin = 5;     // Chân điều khiển hướng cho trục X

// --- LCD 16x4 I2C (PCF8574) ---
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
LiquidCrystal_I2C lcd(0x27, 16, 4);

// --- STATE MACHINE ---
enum SystemState { IDLE, PUMPING };
SystemState systemState = IDLE;

enum HandleState {
  ROTATING,        // Stepper rotating 1 revolution
  SENSOR_CHECK,    // After rotation, check sensor
  PUMP_WAIT,       // Waiting 1s before pump (tube settled)
  PUMP_FILL,       // Pump running, pot time controls duration
  PUMP_DONE,       // Pump stopped, wait 3s
  NO_TUBE_WAIT     // No tube detected, wait 5s
};
HandleState handleState = ROTATING;
unsigned long stateStartTime = 0;
const int PUMP_WAIT_DELAY = 1000;    // 1s before pump
const int POST_PUMP_DELAY = 3000;    // 5s after pump
const int NO_TUBE_DELAY = 5000;     // 5s when no tube
const unsigned long PUMP_TIMEOUT = 5000; // 5s max pump

// --- PUMP STATE ---
unsigned long pumpStartTime = 0;
bool pumpRunning = false;
float capturedProcessTime = 0;  // Locked time when pumping starts

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
int currentSpeed = 0;             // Current PWM value
unsigned long lastDisplayUpdate = 0;

// --- POTENTIOMETER ---
const int speedPot = A1;          // Potentiometer for pump speed control
const int timePot = A2;           // Potentiometer for pump process time (2-20s)

// --- PHOTOELECTRIC SENSOR PNP + TUBE COUNTER ---
const int sensorPin = 10;         // Y+ on CNC shield (D10)
const int resetBtnPin = A0;       // Abort signal (A0)

// --- LIMIT SWITCH X+ (CNC SHIELD V3) ---
const int limitSwitch = 9;       // X+ Limit on CNC Shield V3 (D9)

int tubeCount = 0;
unsigned long lastSensorTrigger = 0;
const int DEBOUNCE_SENSOR = 50;
const int DEBOUNCE_BUTTON = 100;
bool lastAbortBtnState = HIGH;   // For edge detection on latch button

void abortAll() {
  // Stop pump if running
  analogWrite(pumpPWM, 0);
  digitalWrite(pumpDir, LOW);

  // Reset pump state
  pumpRuntime = 0;
  pumpRunning = false;
  currentSpeed = 0;

  // Reset tube count
  tubeCount = 0;

  // Exit any special mode
  systemState = IDLE;

  // Reset handle state to start of sequence
  handleState = ROTATING;

  // Show abort message
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("!! ABORTED !!");
  delay(1000);
}

void homeSeekingSequence() {
  const int HOMING_ROTATIONS = 2;
  const int HOMING_STEPS_PER_ROTATION = 400;
  const unsigned long HOMING_PULSE_US = 5500;
  const unsigned long HOMING_PAUSE_US = 1000;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("HOMING...");

  digitalWrite(dirPin, HIGH);

  for (int rot = 0; rot < HOMING_ROTATIONS; rot++) {
    for (int step = 0; step < HOMING_STEPS_PER_ROTATION; step++) {
      if (digitalRead(limitSwitch) == LOW) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("HOMING DONE!");
        delay(2000);
        lcd.clear();
        int x = 10;
        while(x > 0){
          x -= 1;
          lcd.setCursor(0, 0); 
          lcd.print("STARTING... IN "+ String(x));
          delay(1000);
          lcd.clear();     
        }
        
        return;
      }

      digitalWrite(stepPin, HIGH);
      delayMicroseconds(HOMING_PULSE_US);
      digitalWrite(stepPin, LOW);
      delayMicroseconds(HOMING_PAUSE_US);
    }
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PLEASE TURN OFF");
  lcd.setCursor(9, 1);
  lcd.print("SYSTEM !!!");
  while (1) {
    delay(1000);
  }
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

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("LCD OK");
  delay(1500);



  // Potentiometer
  pinMode(speedPot, INPUT);

  // Photoelectric sensor + reset button + limit switch
  pinMode(sensorPin, INPUT);
  pinMode(resetBtnPin, INPUT_PULLUP);
  pinMode(limitSwitch, INPUT_PULLUP);

  // Homing sequence
  homeSeekingSequence();
}

int readPotSpeed() {
  int potValue = analogRead(speedPot);
  return map(potValue, 0, 1023, 0, 255);
}

float readPotTime() {
  int potValue = analogRead(timePot);
  return map(potValue, 0, 1023, 20, 200) / 10.0;  // 2.0s to 20.0s (decimal)
}

void loop() {
  // --- IDLE: Sensor-driven handle sequence ---
  if (systemState == IDLE) {
    switch (handleState) {
      case ROTATING: {
        // Run stepper 1 revolution
        currentSpeed = 255;
        digitalWrite(dirPin, HIGH);
        for (int x = 0; x < stepsPerRev; x++) {
          digitalWrite(stepPin, HIGH);
          delayMicroseconds(3000);
          digitalWrite(stepPin, LOW);
          delayMicroseconds(100);
          motorSteps++;
        }
        currentSpeed = 0;
        handleState = SENSOR_CHECK;
        stateStartTime = millis();
        break;
      }

      case SENSOR_CHECK: {
        // Check sensor immediately after rotation
        if (digitalRead(sensorPin) == HIGH) {
          // Tube detected - wait 3s then pump
          handleState = PUMP_WAIT;
        } else {
          // No tube - wait 10s
          handleState = NO_TUBE_WAIT;
        }
        stateStartTime = millis();
        break;
      }

      case PUMP_WAIT: {
        // If tube disappears, reset timer (need stable 3s)
        // But if tube never returns within 6s total, abort
        unsigned long waitElapsed = millis() - stateStartTime;
        lcd.setCursor(0, 1);
        lcd.print("PUMP_WAIT:");
        lcd.print(waitElapsed / 1000);
        lcd.print("s     ");
        if (digitalRead(sensorPin) == LOW) {
          stateStartTime = millis();  // Reset timer - wait for stable tube
        } else if (waitElapsed >= PUMP_WAIT_DELAY) {
          // Got stable tube for 3s - start pumping
          lcd.setCursor(0, 1);
          lcd.print("START PUMP!    ");
          handleState = PUMP_FILL;
          stateStartTime = millis();
          // Capture time from potentiometer (locked for this cycle)
          capturedProcessTime = readPotTime();
          // Start pump
          int pumpSpeed = readPotSpeed();
          currentSpeed = pumpSpeed;
          pumpRunning = true;
          pumpStartTime = millis();
          digitalWrite(pumpDir, HIGH);
          analogWrite(pumpPWM, pumpSpeed);
        } else if (waitElapsed >= 6000) {
          // Tube not stable within 6s - abort and rotate
          handleState = ROTATING;
          stateStartTime = millis();
        }
        break;
      }

      case PUMP_FILL: {
        // SAFETY: If tube disappears during pumping, stop pump immediately
        unsigned long elapsed = millis() - stateStartTime;
        if (digitalRead(sensorPin) == LOW) {
          analogWrite(pumpPWM, 0);
          digitalWrite(pumpDir, LOW);
          pumpRuntime += millis() - pumpStartTime;
          pumpRunning = false;
          currentSpeed = 0;
          // Do NOT count - tube disappeared mid-process
          lcd.clear();
          lcd.setCursor(0, 1);
          lcd.print("TUBE LOST! SKIP ");
          delay(1500);
          handleState = ROTATING;
          stateStartTime = millis();
          lcd.clear();
          break;
        }
        // Check timeout ONLY - pot time controls pump duration
        if (millis() - stateStartTime >= (unsigned long)(capturedProcessTime * 1000.0)) {
          // Timeout - stop pump
          analogWrite(pumpPWM, 0);
          digitalWrite(pumpDir, LOW);
          pumpRuntime += (unsigned long)(capturedProcessTime * 1000.0);
          pumpRunning = false;
          currentSpeed = 0;
          tubeCount++;
          handleState = PUMP_DONE;
          stateStartTime = millis();
        }
        break;
      }

      case PUMP_DONE: {
        if (millis() - stateStartTime >= POST_PUMP_DELAY) {
          handleState = ROTATING;
        }
        break;
      }

      case NO_TUBE_WAIT: {
        // Poll sensor during wait - if tube placed, go to pump immediately
        if (digitalRead(sensorPin) == HIGH) {
          handleState = PUMP_WAIT;
          stateStartTime = millis();
        } else if (millis() - stateStartTime >= NO_TUBE_DELAY) {
          handleState = ROTATING;
        }
        break;
      }
    }

    displayUpdate();
  }
}

void runPumpCycle() {
  int pumpSpeed = readPotSpeed();
  float processTime = readPotTime();
  currentSpeed = pumpSpeed;
  pumpRunning = true;
  pumpStartTime = millis();
  unsigned long pumpDuration = (unsigned long)(processTime * 1000.0);
  digitalWrite(pumpDir, HIGH);
  analogWrite(pumpPWM, pumpSpeed);
  displayUpdate();

  while (millis() - pumpStartTime < pumpDuration && pumpRunning) {
    // Check abort
    bool btnState = digitalRead(resetBtnPin);
    if (btnState == LOW && lastAbortBtnState == HIGH) {
      delay(DEBOUNCE_BUTTON);
      if (digitalRead(resetBtnPin) == LOW) {
        analogWrite(pumpPWM, 0);
        digitalWrite(pumpDir, LOW);
        pumpRuntime = 0;
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
    delay(50);
  }

  // Normal stop after pot time
  analogWrite(pumpPWM, 0);
  digitalWrite(pumpDir, LOW);
  pumpRuntime += (unsigned long)(processTime * 1000.0);
  pumpRunning = false;
  currentSpeed = 0;
  displayUpdate();
}

void displayUpdate() {
  // Line 0: "MAY CHIET ARTERMIA"
  lcd.setCursor(0, 0);
  lcd.print("MAY CHIET ARTERMIA");

  // Line 1: "Speed:XXX" - show configured speed from pot
  lcd.setCursor(0, 1);
  lcd.print("Speed:");
  lcd.print(readPotSpeed());
  lcd.print("    ");

  // Line 2: "Tubes:X |....|"
  lcd.setCursor(0, 2);
  lcd.print("Tubes:");
  lcd.print(tubeCount);
  lcd.print("    ");
  if (digitalRead(sensorPin) == HIGH) {
    lcd.print("||||||");
  } else {
    lcd.print("------");
  }

  // Line 3: "Time:X.Xs"
  float potTime = readPotTime();
  lcd.setCursor(0, 3);
  lcd.print("Time:");
  lcd.print(potTime, 1);  // 1 decimal place
  lcd.print("s        ");
}