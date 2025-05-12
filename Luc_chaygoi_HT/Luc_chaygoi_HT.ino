// Sensor pins
int fireSensor = 19;     // D19 - Cảm biến lửa (LOW khi phát hiện lửa)
int smokeSensor = 25;    // D25 - Cảm biến khói (LOW khi phát hiện khói)

// Output pins
int motor = 21;          // D21 - Điều khiển motor bơm nước (qua relay)
int fanRelay = 12;       // D12 - Điều khiển quạt thông gió (qua relay)
int coi = 18;            // D18 - Còi báo động

// LED RGB
int ledR = 14;           // D14 - LED Đỏ
int ledG = 27;           // D27 - LED Xanh lá
int ledB = 26;           // D26 - LED Xanh dương

// UART Communication
#define RXD2 16
#define TXD2 17
bool fireStatus = false;

// Cấu hình SIM800L
#define PHONE_NUMBER "0888124475"  // Thay số điện thoại của bạn ở đây
#define SIM_RX 32                // Chân RX của SIM800L (nối với TX ESP32)
#define SIM_TX 33                  // Chân TX của SIM800L (nối với RX ESP32)
bool callMade = false;             // Cờ kiểm tra đã gọi điện chưa
unsigned long lastCallTime = 0;    // Thời gian gọi cuối cùng
const unsigned long callInterval = 60000; // 60s giữa các lần gọi

// Khởi tạo UART cho SIM800L
HardwareSerial simSerial(1);

void setup() {
  // Thiết lập các chân IO
  pinMode(fireSensor, INPUT_PULLUP);
  pinMode(smokeSensor, INPUT_PULLUP);
  pinMode(coi, OUTPUT);
  pinMode(motor, OUTPUT);
  pinMode(fanRelay, OUTPUT);

  pinMode(ledR, OUTPUT);
  pinMode(ledG, OUTPUT);
  pinMode(ledB, OUTPUT);

  // Khởi tạo UART2
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // Khởi tạo SIM800L
  simSerial.begin(115200, SERIAL_8N1, SIM_RX, SIM_TX);
  delay(200);
  
  // Kiểm tra module SIM
  sim_at_cmd("AT");
  delay(200);
  sim_at_cmd("AT+CPIN?");     // Kiểm tra thẻ SIM
  delay(200);
  sim_at_cmd("AT+CSQ");       // Kiểm tra cường độ sóng

  // Tắt tất cả thiết bị khi khởi động
  turnOffAllDevices();
}

void loop() {
  bool fireDetected = digitalRead(fireSensor) == LOW;
  bool smokeDetected = digitalRead(smokeSensor) == LOW;

  // Gửi trạng thái qua UART nếu có thay đổi
  if (fireDetected != fireStatus) {
    fireStatus = fireDetected;
    Serial2.write(fireStatus ? 'F' : 'N');
  }

  if (fireDetected) {
    // KÍCH HOẠT CHẾ ĐỘ CHỮA CHÁY
    digitalWrite(coi, HIGH);           // Bật còi báo động
    digitalWrite(motor, HIGH);          // Bật motor bơm nước
    digitalWrite(ledR, HIGH);           // Đèn đỏ báo cháy
    
    flashRGB(2);
    // Gọi điện khẩn cấp ngay lập tức
    if (!callMade || (millis() - lastCallTime > callInterval)) {
      flashRGB(2);
      makeEmergencyCall();
      callMade = true;
      lastCallTime = millis();
      
    }

    flashRGB(2); // Nhấp nháy LED cảnh báo

  } else if (smokeDetected) {
    // CHẾ ĐỘ CÓ KHÓI (không có lửa)
    digitalWrite(coi, HIGH);           // Bật còi báo động
    digitalWrite(fanRelay, HIGH);      // Bật quạt thông gió
    digitalWrite(ledR, HIGH);          // Đèn đỏ báo cảnh báo
    
    flashRGB(1); // Nhấp nháy LED cảnh báo
    callMade = false; // Reset trạng thái gọi điện

  } else {
    // TRẠNG THÁI BÌNH THƯỜNG (không có cháy/khói)
    turnOffAllDevices();
    callMade = false; // Reset trạng thái gọi điện
  }

  delay(100); // Chống nhiễu
}

// Hàm tắt tất cả thiết bị
void turnOffAllDevices() {
  digitalWrite(coi, LOW);
  digitalWrite(motor, LOW);
  digitalWrite(fanRelay, LOW);
  digitalWrite(ledR, LOW);
  digitalWrite(ledG, LOW);
  digitalWrite(ledB, LOW);
}

// Hàm nhấp nháy LED RGB
void flashRGB(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(ledR, HIGH);
    delay(200);
    digitalWrite(ledR, LOW);

    digitalWrite(ledG, HIGH);
    delay(200);
    digitalWrite(ledG, LOW);

    digitalWrite(ledB, HIGH);
    delay(200);
    digitalWrite(ledB, LOW);
  }
}

// Hàm xử lý AT command với SIM800L
void sim_at_wait() {
  delay(100);
  while (simSerial.available()) {
    Serial.write(simSerial.read());
  }
}

bool sim_at_cmd(String cmd) {
  simSerial.println(cmd);
  sim_at_wait();
  return true;
}

// Hàm thực hiện cuộc gọi khẩn cấp
void makeEmergencyCall() {
  String temp = "ATD";
  temp += PHONE_NUMBER;
  temp += ";";
  sim_at_cmd(temp); 

  delay(1000);               // Duy trì cuộc gọi 20 giây
  sim_at_cmd("ATH");          // Ngắt cuộc gọi
}