#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
//#include <Servo.h>  // Thêm thư viện Servo
#include <ESP32Servo.h>
#define SS_PIN 16
#define RST_PIN 17
MFRC522 rfid(SS_PIN, RST_PIN);
String uidString;

const char* ssid = "lucluu";  
const char* password = "11111111";
String Web_App_URL = "https://script.google.com/macros/s/AKfycbzTvXSSt7XYkfDp0iRMyE8KdVr9H1_uLnNw2uv-o-_OfQri49yIx4cf_LeJcz2QTjCK/exec";

#define On_Board_LED_PIN 2
#define MAX_STUDENTS 10
struct Student {
  String id;
  char code[10];
  char name[30];
};
Student students[MAX_STUDENTS];
int studentCount = 0;
int runMode = 2;

const int btnIO = 15;
bool btnIOState = HIGH;
unsigned long timeDelay = millis();
unsigned long timeDelay2 = millis();
bool InOutState = 0; // 0: vào, 1: ra
const int ledIO = 4;
const int buzzer = 5;

// Servo setup
Servo servoIn;
Servo servoOut;
#define SERVO_IN_PIN 13
#define SERVO_OUT_PIN 27
#define SERVO_OPEN 90
#define SERVO_CLOSE 0
#define SERVO_OUT_OPEN 90  // Góc mở cổng ra
#define SERVO_OUT_CLOSE 180 // Góc đóng cổng ra
void setup() {
  Serial.begin(115200);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  pinMode(btnIO, INPUT_PULLUP);
  pinMode(ledIO, OUTPUT);
  pinMode(On_Board_LED_PIN, OUTPUT);
  
  servoIn.attach(SERVO_IN_PIN);
  servoOut.attach(SERVO_OUT_PIN);
  servoIn.write(SERVO_CLOSE);
  servoOut.write(SERVO_OUT_CLOSE);
  
  Serial.println();
  Serial.println("-------------");
  Serial.println("WIFI mode : STA");
  Serial.println("-------------");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  if (!readDataSheet()) Serial.println("Can't read data from google sheet!");

  SPI.begin(); 
  rfid.PCD_Init(); 
}

void loop() {
  if (millis() - timeDelay2 > 500) {
    readUID();
    timeDelay2 = millis();
  }

  // Nút bấm cho chế độ ra
  if (digitalRead(btnIO) == LOW) {
    if (btnIOState == HIGH) {
      if (millis() - timeDelay > 500) {
        InOutState = 1; // Chuyển sang chế độ ra
        digitalWrite(ledIO, InOutState);
        Serial.println("Đang ở chế độ RA");
        timeDelay = millis();
      }
      btnIOState = LOW;
    }
  } else {
    btnIOState = HIGH;
  }
}
// n lần lặp
// d delay
void beep(int n, int d) {
  for (int i = 0; i < n; i++) {
    digitalWrite(buzzer, HIGH);
    delay(d);
    digitalWrite(buzzer, LOW);
    delay(d);
  }
}
//Hàm readUID() có chức năng đọc UID của thẻ RFID, lưu lại dưới dạng chuỗi, và thực hiện các hành động dựa trên chế độ hoạt động (runMode).


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
    if (writeLogSheet()) {  // Chỉ mở cổng nếu writeLogSheet() trả về true
      if (InOutState == 0) { // Chế độ vào
        openGateIn();
      } else { // Chế độ ra
        openGateOut();
        InOutState = 0; // Reset về chế độ vào sau khi quét thẻ ra
        digitalWrite(ledIO, LOW);
      }
    }
  }
}

/// Hàm openGateIn() có chức năng điều khiển servo mở cổng vào, giữ trong 3 giây, sau đó đóng lại.

void openGateIn() {
  Serial.println("Mở cổng VÀO");
  servoIn.write(SERVO_OPEN);
  delay(3000); // Mở 3 giây
  servoIn.write(SERVO_CLOSE);
  Serial.println("Đóng cổng VÀO");
}

void openGateOut() {
  Serial.println("Mở cổng RA");
  servoOut.write(SERVO_OUT_OPEN);
  delay(3000); // Mở 3 giây
  servoOut.write(SERVO_OUT_CLOSE);
  Serial.println("Đóng cổng RA");
}

/* ---------- Các hàm xử lý Google Sheet giữ nguyên ---------- */
// ... giữ nguyên các hàm readDataSheet, writeUIDSheet, writeLogSheet, getStudentNameById, countElements, urlencode ...


//Hàm readDataSheet() có nhiệm vụ đọc dữ liệu từ Google Sheets thông qua một Google Apps Script Web App, sau đó phân tích dữ liệu và lưu vào mảng students[].
bool readDataSheet(){
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(On_Board_LED_PIN, HIGH);

    // Create a URL for reading or getting data from Google Sheets.
    String Read_Data_URL = Web_App_URL + "?sts=read";

    Serial.println();
    Serial.println("-------------");
    Serial.println("Read data from Google Spreadsheet...");
    Serial.print("URL : ");
    Serial.println(Read_Data_URL);

    //::::::::::::::::::The process of reading or getting data from Google Sheets.
      // Initialize HTTPClient as "http".
      HTTPClient http;

      // HTTP GET Request.
      http.begin(Read_Data_URL.c_str());
      http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

      // Gets the HTTP status code.
      int httpCode = http.GET(); 
      Serial.print("HTTP Status Code : ");
      Serial.println(httpCode);
  
      // Getting response from google sheet.
      String payload;
      studentCount=0;
      if (httpCode > 0) {
        payload = http.getString();
        Serial.println("Payload : " + payload); 

         // Tách dữ liệu
          char charArray[payload.length() + 1];
          payload.toCharArray(charArray, payload.length() + 1);
          // Đếm số phần tử
          int numberOfElements = countElements(charArray, ',');
          Serial.println("Number of elements: "+String(numberOfElements));
          char *token = strtok(charArray, ",");
          while (token != NULL && studentCount < numberOfElements/3) {
            //---------------------------------
            students[studentCount].id = atoi(token); // Chuyển đổi ID từ chuỗi sang số nguyên
            token = strtok(NULL, ",");
            strcpy(students[studentCount].code, token); // Sao chép mã học sinh
            token = strtok(NULL, ",");
            strcpy(students[studentCount].name, token); // Sao chép tên học sinh
            
            studentCount++;
            token = strtok(NULL, ",");
          }
          
          // In ra danh sách học sinh
          for (int i = 0; i < studentCount; i++) {
            Serial.print("ID: ");
            Serial.println(students[i].id);
            Serial.print("Code: ");
            Serial.println(students[i].code);
            Serial.print("Name: ");
            Serial.println(students[i].name);
          }
      }
  
      http.end();
    //::::::::::::::::::
    
    digitalWrite(On_Board_LED_PIN, LOW);
    Serial.println("-------------");
    if(studentCount>0) return true;
    else return false;
  }
}

///Hàm writeUIDSheet() có nhiệm vụ gửi UID của thẻ RFID vừa quét được lên Google Sheets thông qua Google Apps Script Web App.
void writeUIDSheet(){
  String Send_Data_URL = Web_App_URL + "?sts=writeuid";
  Send_Data_URL += "&uid=" + uidString;
  Serial.println();
  Serial.println("-------------");
  Serial.println("Send data to Google Spreadsheet...");
  Serial.print("URL : ");
  Serial.println(Send_Data_URL);
  // Initialize HTTPClient as "http".
  HTTPClient http;

  // HTTP GET Request.
  http.begin(Send_Data_URL.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  // Gets the HTTP status code.
  int httpCode = http.GET(); 
  Serial.print("HTTP Status Code : ");
  Serial.println(httpCode);

  // Getting response from google sheets.
  String payload;
  if (httpCode > 0) {
    payload = http.getString();
    Serial.println("Payload : " + payload);    
  }
  
  http.end();
}
//Hàm writeLogSheet() ghi nhật ký quét thẻ RFID lên Google Sheets, bao gồm:
bool writeLogSheet() {
  char charArray[uidString.length() + 1];
  uidString.toCharArray(charArray, uidString.length() + 1);
  char* studentName = getStudentNameById(charArray);

  if (studentName != nullptr) {
    Serial.print("Tên chủ ID ");
    Serial.print(uidString);
    Serial.print(" là: ");
    Serial.println(studentName);

    String Send_Data_URL = Web_App_URL + "?sts=writelog";
    Send_Data_URL += "&uid=" + uidString;
    Send_Data_URL += "&name=" + urlencode(String(studentName));
    Send_Data_URL += "&inout=" + urlencode(InOutState == 0 ? "VÀO" : "RA");

    Serial.println("Gửi dữ liệu đến Google Sheets...");
    HTTPClient http;
    http.begin(Send_Data_URL.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET(); 
    Serial.print("HTTP Status Code : ");
    Serial.println(httpCode);
    http.end();

    return true;  // Trả về true nếu tìm thấy và ghi log thành công
  } else {
    Serial.print("Không tìm thấy chủ thẻ với ID ");
    Serial.println(uidString);
    beep(3, 500);  // Cảnh báo bằng tiếng beep
    return false;  // Trả về false nếu không tìm thấy chủ thẻ
  }
}

///Hàm urlencode() được sử dụng để mã hóa một chuỗi (String) thành định dạng URL (URL encoding).
//Điều này rất quan trọng khi gửi dữ liệu lên Google Sheets qua HTTP GET request, vì một số ký tự đặc biệt như dấu cách ( ), dấu &, ?, %, v.v. có thể gây lỗi trong URL.

String urlencode(String str) {
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
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
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
    }
    yield();
  }
  return encodedString;
}
//Hàm getStudentNameById() dùng để tìm tên sinh viên dựa trên UID thẻ RFID.
//📌 Nếu UID tìm thấy trong danh sách sinh viên (students[]), hàm trả về con trỏ đến tên sinh viên.
//📌 Nếu UID không tồn tại, hàm trả về nullptr.
char* getStudentNameById(char* uid) {
  for (int i = 0; i < studentCount; i++) {
    if (strcmp(students[i].code, uid) == 0) {
      return students[i].name;
    }
  }
  return nullptr; // Trả về nullptr nếu không tìm thấy
}

//Hàm countElements() dùng để đếm số lượng phần tử trong một chuỗi được phân tách bởi ký tự delimiter (ví dụ: dấu phẩy ,).
int countElements(const char* data, char delimiter) {
  // Tạo một bản sao của chuỗi dữ liệu để tránh thay đổi chuỗi gốc
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