#include <Wire.h>  // Thư viện hỗ trợ giao tiếp I2C
#include <LiquidCrystal_I2C.h>  // Thư viện điều khiển màn hình LCD I2C

#define NUM_SENSORS 6  // Định nghĩa số lượng cảm biến hồng ngoại

// Khởi tạo đối tượng LCD với địa chỉ I2C 0x27, kích thước 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Mảng chứa các chân GPIO kết nối với cảm biến hồng ngoại
const int ir_pins[NUM_SENSORS] = {5, 6, 7, 8, 9, 10};
// Mảng lưu trạng thái của từng cảm biến (0: trống, 1: có xe)
int sensors[NUM_SENSORS] = {0};
// Biến đếm số chỗ trống trong bãi đỗ xe
int empty_slots = NUM_SENSORS;

void setup() {
  lcd.init();  // Khởi động LCD
  lcd.backlight();  // Bật đèn nền LCD

  Serial.begin(9600);  // Khởi động giao tiếp Serial với tốc độ 9600 baud

  // Cấu hình tất cả chân cảm biến làm INPUT
  for (int i = 0; i < NUM_SENSORS; i++) {
    pinMode(ir_pins[i], INPUT);
  }

  // Hiển thị màn hình khởi động trên LCD
  lcd.setCursor(0, 0);
  lcd.print("   Parking Sys   ");  // Hiển thị dòng tiêu đề
  lcd.setCursor(0, 1);
  lcd.print("   Loading...    ");  // Hiển thị trạng thái khởi động
  delay(2000);  // Đợi 2 giây trước khi xóa màn hình LCD
  lcd.clear();

  // Đọc cảm biến nhiều lần lúc đầu để ổn định dữ liệu
  for (int i = 0; i < 3; i++) { // Đọc 3 lần để lọc nhiễu
    Read_Sensors();
    delay(500);  // Chờ 500ms giữa mỗi lần đọc
  }
}

void loop() {
  Read_Sensors();  // Cập nhật trạng thái cảm biến

  // Đếm số chỗ đã bị chiếm
  int occupied = 0;
  for (int i = 0; i < NUM_SENSORS; i++) {
    occupied += sensors[i];  // Cộng dồn số cảm biến đang báo có xe
  }
  empty_slots = NUM_SENSORS - occupied;  // Tính số chỗ trống còn lại

  Display_Status();  // Cập nhật thông tin lên LCD

  // Gửi dữ liệu debug lên Serial Monitor
  Serial.print("Empty Slots: ");
  Serial.println(empty_slots);

  delay(700);  // Đợi 700ms để tránh màn hình bị nhấp nháy
}

// Hàm đọc cảm biến với cơ chế lọc nhiễu
void Read_Sensors() {
  for (int i = 0; i < NUM_SENSORS; i++) {  // Duyệt qua tất cả các cảm biến
    int count = 0;  // Biến đếm số lần phát hiện có xe
    for (int j = 0; j < 5; j++) { // Đọc giá trị cảm biến 5 lần
      if (digitalRead(ir_pins[i]) == LOW) {  // Cảm biến LOW nghĩa là có xe
        count++;  // Tăng biến đếm nếu có xe
      }
      delay(5);  // Đợi 5ms giữa các lần đọc
    }
    sensors[i] = (count >= 3) ? 1 : 0;  // Nếu đọc được ít nhất 3/5 lần có xe -> xác nhận có xe
  }
}

// Hàm hiển thị trạng thái bãi đỗ xe lên LCD
void Display_Status() {
  lcd.setCursor(0, 0);
  lcd.print("Slots Empty: ");
  lcd.print(empty_slots);  // Hiển thị số chỗ trống còn lại
  lcd.print("  ");  // Xóa ký tự dư trên màn hình

  lcd.setCursor(0, 1);
  // Hiển thị trạng thái của 3 cảm biến đầu tiên
  for (int i = 0; i < 3; i++) {
    lcd.print("S"); lcd.print(i+1); lcd.print(":");
    lcd.print(sensors[i] ? "F " : "E ");  // 'F' nếu có xe, 'E' nếu trống
  }

  lcd.setCursor(0, 2);
  // Hiển thị trạng thái của 3 cảm biến còn lại
  for (int i = 3; i < NUM_SENSORS; i++) {
    lcd.print("S"); lcd.print(i+1); lcd.print(":");
    lcd.print(sensors[i] ? "F " : "E ");  // 'F' nếu có xe, 'E' nếu trống
  }
}
