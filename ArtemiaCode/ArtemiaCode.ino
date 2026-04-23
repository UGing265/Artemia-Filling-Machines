// --- ĐỊNH NGHĨA CHÂN CHO TRỤC Z ---
const int stepPin = 2;    // Chân phát xung cho trục X
const int dirPin = 5;     // Chân điều khiển hướng cho trục X

// --- CẤU HÌNH THÔNG SỐ MOTOR ---
// Khi chỉ cắm Jumper M0 -> Chế độ 1/2 Step -> 400 bước/vòng
const int stepsPerRev = 800;

void setup() {
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  // Không cần pinMode chân 8 hay digitalWrite chân 8 vì đã cắm jumper EN/GND cứng trên mạch
}

void loop() {
  // QUAY THUẬN 1 VÒNG
  digitalWrite(dirPin, HIGH);
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(1); // Ở chế độ Full Step, để 2000-4000 là vừa
    digitalWrite(stepPin, LOW);
    delayMicroseconds(3000);
  }
  
  delay(2000); // Nghỉ 2 giây
  
  // QUAY NGƯỢC 1 VÒNG
  digitalWrite(dirPin, LOW);
  for(int x = 0; x < stepsPerRev; x++) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(100);
    digitalWrite(stepPin, LOW);
    delayMicroseconds(3000);
  }
  
  delay(2000);
}