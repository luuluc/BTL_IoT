#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Servo.h>

// Cấu hình RFID
#define SS_PIN 16   // GPIO5 (D5) - Thay đổi vì GPIO16 đã dùng cho UART
#define RST_PIN 17 // GPIO17 (D17)
MFRC522 rfid(SS_PIN, RST_PIN);
String uidString;

// Cấu hình WiFi
const char* ssid = "lucluu";  
const char* password = "11111111";
String Web_App_URL = "https://script.google.com/macros/s/AKfycbzTvXSSt7XYkfDp0iRMyE8KdVr9H1_uLnNw2uv-o-_OfQri49yIx4cf_LeJcz2QTjCK/exec";

// Cấu hình hệ thống
#define On_Board_LED_PIN 2
#define MAX_STUDENTS 10
struct Student {
  String id;
  char code[10];
  char name[30];
};
Student students[MAX_STUDENTS];
int studentCount = 0;
int runMode = 2; // 1: Ghi UID, 2: Chế độ hoạt động

// Các chân I/O
const int btnIO = 15;          // GPIO15 - Nút chuyển chế độ vào/ra
bool btnIOState = HIGH;
unsigned long timeDelay = millis();
unsigned long timeDelay2 = millis();
bool InOutState = 0;           // 0: vào, 1: ra
const int ledIO = 4;           // GPIO4 - Đèn báo chế độ
const int buzzer = 5;          // GPIO5 - Còi báo

// Cấu hình Servo
Servo servoIn;
Servo servoOut;
#define SERVO_IN_PIN 13        // GPIO13 - Servo cổng vào
#define SERVO_OUT_PIN 27       // GPIO27 - Servo cổng ra
#define SERVO_OPEN_ANGLE 90    // Góc mở cổng vào
#define SERVO_CLOSE_ANGLE 0    // Góc đóng cổng vào
#define SERVO_OUT_OPEN_ANGLE 90  // Góc mở cổng ra
#define SERVO_OUT_CLOSE_ANGLE 180 // Góc đóng cổng ra

// Cấu hình UART
#define RXD2 14              // GPIO16 (RX2)
#define TXD2 12                // GPIO12 (TX2)
bool emergencyMode = false;
unsigned long emergencyStartTime = 0;
const long emergencyDuration = 30000; // 30 giây trong chế độ khẩn cấp

void setup() {
  Serial.begin(115200);
  
  // Thiết lập chân I/O
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  pinMode(btnIO, INPUT_PULLUP);
  pinMode(ledIO, OUTPUT);
  pinMode(On_Board_LED_PIN, OUTPUT);
  
  // Khởi tạo Servo
  servoIn.attach(SERVO_IN_PIN);
  servoOut.attach(SERVO_OUT_PIN);
  closeAllGates();
  
  // Khởi tạo UART2
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  // Kết nối WiFi
  Serial.println("\n-------------");
  Serial.println("WIFI mode : STA");
  Serial.println("-------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected to WiFi");
  
  // Đọc dữ liệu từ Google Sheet
  if (!readDataSheet()) {
    Serial.println("Không thể đọc dữ liệu từ Google Sheet!");
  }

  // Khởi tạo RFID
  SPI.begin(); 
  rfid.PCD_Init(); 
  Serial.println("Hệ thống cổng đã sẵn sàng");
}

void loop() {
  // Kiểm tra UART để nhận tín hiệu cháy
  if (Serial2.available()) {
    char received = Serial2.read();
    if (received == 'F') {
      if (!emergencyMode) {
        emergencyMode = true;
        emergencyStartTime = millis();
        Serial.println("Kích hoạt chế độ khẩn cấp: Mở tất cả cổng!");
        openAllGates();
      }
    } else if (received == 'N') {
      if (emergencyMode) {
        emergencyMode = false;
        Serial.println("Kết thúc chế độ khẩn cấp");
        closeAllGates();
      }
    }
  }

  // Tự động thoát chế độ khẩn cấp sau 30s nếu không nhận tín hiệu mới
  if (emergencyMode && (millis() - emergencyStartTime > emergencyDuration)) {
    emergencyMode = false;
    Serial.println("Tự động thoát chế độ khẩn cấp sau 30s");
    closeAllGates();
  }

  // Trong chế độ bình thường, hoạt động như cũ
  if (!emergencyMode) {
    if (millis() - timeDelay2 > 500) {
      readUID();
      timeDelay2 = millis();
    }

    // Xử lý nút bấm chuyển chế độ
    if (digitalRead(btnIO) == LOW) {
      if (btnIOState == HIGH) {
        if (millis() - timeDelay > 500) {
          InOutState = !InOutState; // Đảo trạng thái
          digitalWrite(ledIO, InOutState);
          Serial.println(InOutState ? "Chuyển sang chế độ RA" : "Chuyển sang chế độ VÀO");
          timeDelay = millis();
        }
        btnIOState = LOW;
      }
    } else {
      btnIOState = HIGH;
    }
  }
}

// Mở tất cả cổng trong chế độ khẩn cấp
void openAllGates() {
  Serial.println("Mở tất cả cổng");
  servoIn.write(SERVO_OPEN_ANGLE);
  servoOut.write(SERVO_OUT_OPEN_ANGLE);
  digitalWrite(buzzer, HIGH);
  digitalWrite(ledIO, HIGH);
  beep(3, 200);
}

// Đóng tất cả cổng
void closeAllGates() {
  Serial.println("Đóng tất cả cổng");
  servoIn.write(SERVO_CLOSE_ANGLE);
  servoOut.write(SERVO_OUT_CLOSE_ANGLE);
  digitalWrite(buzzer, LOW);
  digitalWrite(ledIO, LOW);
  beep(1, 200);
}

// Hàm bíp còi
void beep(int n, int d) {
  for (int i = 0; i < n; i++) {
    digitalWrite(buzzer, HIGH);
    delay(d);
    digitalWrite(buzzer, LOW);
    delay(d);
  }
}

// Đọc UID thẻ RFID
void readUID() {
  if (!rfid.PICC_IsNewCardPresent()) return;
  if (!rfid.PICC_ReadCardSerial()) return;

  uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    uidString.concat(String(rfid.uid.uidByte[i] < 0x10 ? "0" : ""));
    uidString.concat(String(rfid.uid.uidByte[i], HEX));
  }
  uidString.toUpperCase();
  Serial.println("Card UID: " + uidString);
  beep(1, 200);

  if (runMode == 1) {
    writeUIDSheet();
  } else if (runMode == 2) {
    if (writeLogSheet()) {
      if (InOutState == 0) {
        openGateIn();
      } else {
        openGateOut();
      }
    }
  }
}

// Mở cổng vào
void openGateIn() {
  Serial.println("Mở cổng VÀO");
  servoIn.write(SERVO_OPEN_ANGLE);
  delay(3000);
  servoIn.write(SERVO_CLOSE_ANGLE);
  Serial.println("Đóng cổng VÀO");
}

// Mở cổng ra
void openGateOut() {
  Serial.println("Mở cổng RA");
  servoOut.write(SERVO_OUT_OPEN_ANGLE);
  delay(3000);
  servoOut.write(SERVO_OUT_CLOSE_ANGLE);
  Serial.println("Đóng cổng RA");
  InOutState = 0;
  digitalWrite(ledIO, LOW);
}

/* ---------- Các hàm xử lý Google Sheet ---------- */
bool readDataSheet() {
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(On_Board_LED_PIN, HIGH);
    
    String Read_Data_URL = Web_App_URL + "?sts=read";
    Serial.println("\n-------------");
    Serial.println("Đọc dữ liệu từ Google Sheet...");
    Serial.print("URL: ");
    Serial.println(Read_Data_URL);

    HTTPClient http;
    http.begin(Read_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);
  
    String payload;
    studentCount = 0;
    if (httpCode > 0) {
      payload = http.getString();
      Serial.println("Payload: " + payload); 

      char charArray[payload.length() + 1];
      payload.toCharArray(charArray, payload.length() + 1);
      
      int numberOfElements = countElements(charArray, ',');
      Serial.println("Số phần tử: " + String(numberOfElements));
      
      char *token = strtok(charArray, ",");
      while (token != NULL && studentCount < numberOfElements/3) {
        students[studentCount].id = atoi(token);
        token = strtok(NULL, ",");
        strcpy(students[studentCount].code, token);
        token = strtok(NULL, ",");
        strcpy(students[studentCount].name, token);
        
        studentCount++;
        token = strtok(NULL, ",");
      }
      
      // In danh sách học sinh
      for (int i = 0; i < studentCount; i++) {
        Serial.print("ID: ");
        Serial.print(students[i].id);
        Serial.print(" | Code: ");
        Serial.print(students[i].code);
        Serial.print(" | Name: ");
        Serial.println(students[i].name);
      }
    }
  
    http.end();
    digitalWrite(On_Board_LED_PIN, LOW);
    Serial.println("-------------");
    
    return studentCount > 0;
  }
  return false;
}

void writeUIDSheet() {
  String Send_Data_URL = Web_App_URL + "?sts=writeuid";
  Send_Data_URL += "&uid=" + uidString;
  
  Serial.println("\n-------------");
  Serial.println("Gửi UID lên Google Sheet...");
  Serial.print("URL: ");
  Serial.println(Send_Data_URL);
  
  HTTPClient http;
  http.begin(Send_Data_URL.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  int httpCode = http.GET(); 
  Serial.print("HTTP Status Code: ");
  Serial.println(httpCode);

  String payload;
  if (httpCode > 0) {
    payload = http.getString();
    Serial.println("Payload: " + payload);    
  }
  
  http.end();
}

bool writeLogSheet() {
  char charArray[uidString.length() + 1];
  uidString.toCharArray(charArray, uidString.length() + 1);
  char* studentName = getStudentNameById(charArray);

  if (studentName != nullptr) {
    Serial.print("Tên chủ thẻ ");
    Serial.print(uidString);
    Serial.print(": ");
    Serial.println(studentName);

    String Send_Data_URL = Web_App_URL + "?sts=writelog";
    Send_Data_URL += "&uid=" + uidString;
    Send_Data_URL += "&name=" + urlencode(String(studentName));
    Send_Data_URL += "&inout=" + urlencode(InOutState == 0 ? "VÀO" : "RA");

    Serial.println("Gửi dữ liệu log lên Google Sheet...");
    HTTPClient http;
    http.begin(Send_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);
    http.end();

    return true;
  } else {
    Serial.print("Không tìm thấy chủ thẻ với UID ");
    Serial.println(uidString);
    beep(3, 500);
    return false;
  }
}

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
}

char* getStudentNameById(char* uid) {
  for (int i = 0; i < studentCount; i++) {
    if (strcmp(students[i].code, uid) == 0) {
      return students[i].name;
    }
  }
  return nullptr;
}

int countElements(const char* data, char delimiter) {
  char dataCopy[strlen(data) + 1];
  strcpy(dataCopy, data);
  
  int count = 0;
  char* token = strtok(dataCopy, &delimiter);
  while (token != NULL) {
    count++;
    token = strtok(NULL, &delimiter);
  }
  return count;
}