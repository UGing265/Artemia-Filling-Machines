// --- ĐỊNH NGHĨA CHÂN CHO TRỤC Z ---
const int stepPin = 2;    // Chân phát xung cho trục X
const int dirPin = 5;     // Chân điều khiển hướng cho trục X

// --- LCD 16x4 I2C (PCF8574) ---
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 4);


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
bool pumpRunning = false;         // Pump state flag
unsigned long pumpStartTime = 0;  // For runtime tracking
const float FLOW_RATE = 0.5;      // ml per second (calibration constant)
unsigned long lastDisplayUpdate = 0;

// --- POTENTIOMETER ---
const int speedPot = A1;          // Potentiometer for pump speed control

// --- PHOTOELECTRIC SENSOR PNP + TUBE COUNTER ---
const int sensorPin = 10;         // Y+ on CNC shield (D10)
const int resetBtnPin = A0;       // Abort signal (A0)
int tubeCount = 0;
unsigned long lastSensorTrigger = 0;
const int DEBOUNCE_SENSOR = 50;
const int DEBOUNCE_BUTTON = 100;

// --- AUTO-STOP PUMP ---
const float TARGET_VOLUME = 5.0;

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

  // LCD Init
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("LCD OK");

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
  // --- SENSOR DETECTION + RESET BUTTON (continuous) ---
  if (digitalRead(sensorPin) == HIGH &&
      millis() - lastSensorTrigger > DEBOUNCE_SENSOR) {
    tubeCount++;
    lastSensorTrigger = millis();
  }

  if (digitalRead(resetBtnPin) == LOW) {
    tubeCount = 0;
    delay(DEBOUNCE_BUTTON);
  }

  // QUAY THUẬN 1 VÒNG
  currentSpeed = 255; // Motor running at full
  digitalWrite(dirPin, HIGH);
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(3000); // Ở chế độ Full Step, để 2000-4000 là vừa
    digitalWrite(stepPin, LOW);
    delayMicroseconds(100);
    motorSteps++;
  }

  delay(1000); // Nghỉ 2 giây
  currentSpeed = 0; // Motor stopped

  // QUAY NGƯỢC 1 VÒNG
  // digitalWrite(dirPin, LOW);
  // for(int x = 0; x < stepsPerRev; x++) {
  //   digitalWrite(stepPin, HIGH);
  //   delayMicroseconds(100);
  //   digitalWrite(stepPin, LOW);
  //   delayMicroseconds(3000);
  // }

  // Pump with potentiometer speed control
  int pumpSpeed = readPotSpeed();
  currentSpeed = pumpSpeed;
  pumpRunning = true;
  pumpStartTime = millis();
  digitalWrite(pumpDir, HIGH);
  digitalWrite(pumpPWM, HIGH);
  analogWrite(pumpPWM, pumpSpeed);
  displayUpdate(); // Show speed while pumping
  delay(2000);

  // Auto-stop check: if volume >= 5ml, stop pump immediately
  float currentVolume = (millis() - pumpStartTime) / 1000.0 * FLOW_RATE;
  if (currentVolume >= TARGET_VOLUME) {
    analogWrite(pumpPWM, 0);  // Stop pump
    pumpRuntime += millis() - pumpStartTime;
    pumpVolume = pumpRuntime / 1000.0 * FLOW_RATE;
    pumpRunning = false;
    currentSpeed = 0;
    displayUpdate(); // Final update
  } else {
    analogWrite(pumpPWM, 0);
    pumpRuntime += millis() - pumpStartTime;
    pumpVolume = pumpRuntime / 1000.0 * FLOW_RATE;
    pumpRunning = false;
    currentSpeed = 0;
    displayUpdate(); // Final update
  }

  // Update LCD every 500ms during idle
  if (millis() - lastDisplayUpdate >= 500) {
    displayUpdate();
    lastDisplayUpdate = millis();
  }
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