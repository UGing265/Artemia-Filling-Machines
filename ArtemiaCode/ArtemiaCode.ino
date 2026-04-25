// --- ĐỊNH NGHĨA CHÂN CHO TRỤC Z ---
const int stepPin = 2;    // Chân phát xung cho trục X
const int dirPin = 5;     // Chân điều khiển hướng cho trục X


// --- ĐỊNH NGHĨA CHÂN CHO BƠM NHU ĐỘNG (L298N) ---
const int pumpPWM = 11;   // Chân ENA của L298N nối vào Z+ trên Shield (D11)
const int pumpDir = 13;   // Chân IN1 của L298N nối vào SpnDir trên Shield (D13)
// Lưu ý: Chân IN2 của L298N nối trực tiếp vào GND

// --- CẤU HÌNH THÔNG SỐ MOTOR ---
// Khi chỉ cắm Jumper M0 -> Chế độ 1/2 Step -> 400 bước/vòng
const int stepsPerRev = 100; // 800 step con 200 step là 90 độ

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
}

void loop() {
  // QUAY THUẬN 1 VÒNG
  digitalWrite(dirPin, HIGH);
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(3000); // Ở chế độ Full Step, để 2000-4000 là vừa
    digitalWrite(stepPin, LOW);
    delayMicroseconds(100);
  }
  
  delay(1000); // Nghỉ 2 giây
  
  // QUAY NGƯỢC 1 VÒNG
  // digitalWrite(dirPin, LOW);
  // for(int x = 0; x < stepsPerRev; x++) {
  //   digitalWrite(stepPin, HIGH);
  //   delayMicroseconds(100);
  //   digitalWrite(stepPin, LOW);
  //   delayMicroseconds(3000);
  // }
  digitalWrite(pumpDir, HIGH);
  digitalWrite(pumpPWM, HIGH);
  analogWrite(pumpPWM, 255);       // Bật bơm với tốc độ ~80% (0-255)
  delay(2000);
  analogWrite(pumpPWM, 0);

}