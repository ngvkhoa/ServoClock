#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <RTClib.h>

// Khởi tạo RTC DS3231 (Địa chỉ I2C mặc định 0x68)
RTC_DS3231 rtc;

// Khởi tạo 2 board PCA9685
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40); // 2 số GIỜ (bên trái)
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41); // 2 số PHÚT (bên phải)

#define SERVO_FREQ 50

// Xung PWM chuẩn cho SG90/MG90S ở 50Hz (chu kỳ 20ms)
const uint16_t PULSE_MIN = 102; // ~0 độ
const uint16_t PULSE_MAX = 512; // ~180 độ

// Cấu hình góc cho từng thanh: a, b, c, d, e, f, g (index 0 đến 6)
// Mở toàn bộ ở 90 độ. Đóng: c(2), d(3), f(5) ở 180 độ; các nét còn lại ở 0 độ.
const uint8_t ANGLE_ON[7]  = { 90,   90,  90,  90,  90,  90,  90};
const uint8_t ANGLE_OFF[7] = {  0,    0, 180, 180,   0, 180,   0};

// Kênh vật lý tương ứng 7 đoạn cho từng chữ số trên mỗi board
// HÀNG CHỤC (bên trái của mỗi board): kênh 9 - 15
const uint8_t DIGIT_TENS_CH[7]  = {9, 10, 11, 12, 13, 14, 15};
// HÀNG ĐƠN VỊ (bên phải của mỗi board): kênh 0 - 6
const uint8_t DIGIT_UNITS_CH[7] = {0,  1,  2,  3,  4,  5,  6};

// Bảng mã 7 đoạn số 0 - 9 (Bit: g f e d c b a)
const uint8_t NUM_MAP[10] = {
  0b00111111, // 0: a, b, c, d, e, f
  0b00000110, // 1: b, c
  0b01011011, // 2: a, b, d, e, g
  0b01001111, // 3: a, b, c, d, g
  0b01100110, // 4: b, c, f, g
  0b01101101, // 5: a, c, d, f, g
  0b01111101, // 6: a, c, d, e, f, g
  0b00000111, // 7: a, b, c
  0b01111111, // 8: a, b, c, d, e, f, g
  0b01101111  // 9: a, b, c, d, f, g
};

// Lưu ký tự đang hiển thị: [0, 1] trên 0x40 (Giờ), [2, 3] trên 0x41 (Phút)
char currentDisplay[4] = {'\0', '\0', '\0', '\0'};

// Biến quản lý chu kỳ đọc RTC không gây block
unsigned long lastRtcCheck = 0;

// Chuyển góc (độ) sang giá trị tick xung PWM
uint16_t angleToPulse(float degrees) {
  degrees = constrain(degrees, 0.0f, 180.0f);
  return (uint16_t)map(degrees, 0, 180, PULSE_MIN, PULSE_MAX);
}

// Lấy bitmask 7 đoạn cho ký tự
uint8_t getCharPattern(char c) {
  if (c >= '0' && c <= '9') return NUM_MAP[c - '0'];
  switch (c) {
    case 'A': case 'a': return 0b01110111;
    case 'B': case 'b': return 0b01111100;
    case 'C':           return 0b00111001;
    case 'D': case 'd': return 0b01011110;
    case 'E': case 'e': return 0b01111001;
    case 'F': case 'f': return 0b01110001;
    case 'H':           return 0b01110110;
    case 'L': case 'l': return 0b00111000;
    case 'P': case 'p': return 0b01110011;
    case 'U':           return 0b00111110;
    case '-':           return 0b01000000;
    case ' ':           return 0b00000000;
    default:            return 0b00000000;
  }
}

// Điều khiển một thanh cụ thể trên board tương ứng
void driveSegment(Adafruit_PWMServoDriver &board, const uint8_t* pinMap, uint8_t segIndex, bool state) {
  uint8_t angle = state ? ANGLE_ON[segIndex] : ANGLE_OFF[segIndex];
  board.setPWM(pinMap[segIndex], 0, angleToPulse(angle));
}

// Cập nhật 1 chữ số theo quy trình an toàn 3 bước: Thu ngoài -> Chỉnh g -> Bung ngoài
// digitPos: 0 (0x40 Chục giờ), 1 (0x40 Đơn vị giờ), 2 (0x41 Chục phút), 3 (0x41 Đơn vị phút)
void updateDigit(uint8_t digitPos, char c) {
  // NẾU SỐ KHÔNG THAY ĐỔI -> BỎ QUA NGAY
  if (currentDisplay[digitPos] == c) return;

  // Xác định board và chân cắm
  Adafruit_PWMServoDriver &board = (digitPos < 2) ? pwm1 : pwm2;
  const uint8_t* pinMap = (digitPos % 2 == 0) ? DIGIT_TENS_CH : DIGIT_UNITS_CH;

  uint8_t pattern = getCharPattern(c);

  // BƯỚC 1: Dọn đường - Thu toàn bộ 6 nét ngoài (a -> f) về vị trí đóng
  for (uint8_t s = 0; s < 6; s++) {
    driveSegment(board, pinMap, s, false);
  }
  delay(70); // Chờ các nét ngoài dạt vào hoàn toàn

  // BƯỚC 2: Đưa nét g (index 6) về trạng thái đích của số mới
  bool state_g = (pattern >> 6) & 0x01;
  driveSegment(board, pinMap, 6, state_g);
  delay(70); // Chờ nét g định vị xong

  // BƯỚC 3: Dựng 6 nét ngoài (a -> f) theo đúng số mới
  for (uint8_t s = 0; s < 6; s++) {
    bool state = (pattern >> s) & 0x01;
    driveSegment(board, pinMap, s, state);
  }

  // Cập nhật trạng thái hiển thị
  currentDisplay[digitPos] = c;
}

// Hiển thị chuỗi 4 ký tự
void display4Digits(const char* str) {
  for (uint8_t i = 0; i < 4; i++) {
    updateDigit(i, str[i]);
  }
}

// Đọc thời gian thực từ DS3231 và đẩy ra 4 chữ số
void updateClockFromRTC() {
  DateTime now = rtc.now();

  char timeStr[5];
  // Chuẩn 24 giờ: Giờ từ 00 - 23, Phút từ 00 - 59
  timeStr[0] = (now.hour() / 10) + '0';
  timeStr[1] = (now.hour() % 10) + '0';
  timeStr[2] = (now.minute() / 10) + '0';
  timeStr[3] = (now.minute() % 10) + '0';
  timeStr[4] = '\0';

  // Chỉ in log khi có thay đổi hiển thị
  if (timeStr[0] != currentDisplay[0] || timeStr[1] != currentDisplay[1] ||
      timeStr[2] != currentDisplay[2] || timeStr[3] != currentDisplay[3]) {
    Serial.print("[RTC 24H] ");
    Serial.print(timeStr[0]); Serial.print(timeStr[1]);
    Serial.print(":");
    Serial.print(timeStr[2]); Serial.println(timeStr[3]);
    
    display4Digits(timeStr);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Khởi động RTC DS3231
  if (!rtc.begin()) {
    Serial.println("Loi: Khong tim thay RTC DS3231 tai 0x68!");
  } else {
    Serial.println("RTC DS3231 san sang.");
    if (rtc.lostPower()) {
      Serial.println("RTC mat nguon, tu dong nap thoi gian bien dich code.");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // Khởi động 2 board PCA9685
  pwm1.begin();
  pwm1.setPWMFreq(SERVO_FREQ);

  pwm2.begin();
  pwm2.setPWMFreq(SERVO_FREQ);

  Serial.println("\n=== DONG HO 7 DOAN SERVO 24H SAN SANG ===");
  Serial.println("Board 0x40: GIO (tens: 9-15, units: 0-6)");
  Serial.println("Board 0x41: PHUT (tens: 9-15, units: 0-6)");
  Serial.println("Co the go 'SET HHMM' de chinh lai gio RTC (Vi du: SET 1430)");
  Serial.println("==========================================");

  // Khởi tạo: đóng toàn bộ 28 servo
  display4Digits("    ");
  delay(500);

  // Đọc và hiển thị giờ ngay khi khởi động
  updateClockFromRTC();
}

void loop() {
  // Kiểm tra thời gian từ RTC mỗi 500ms
  if (millis() - lastRtcCheck >= 500) {
    lastRtcCheck = millis();
    updateClockFromRTC();
  }

  // Lệnh chỉnh giờ thủ công qua Serial nếu RTC sai
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input.startsWith("SET ") && input.length() == 8) {
      // Định dạng lệnh: SET 1430
      int h = input.substring(4, 6).toInt();
      int m = input.substring(6, 8).toInt();

      if (h >= 0 && h < 24 && m >= 0 && m < 60) {
        DateTime now = rtc.now();
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, 0));
        Serial.print("Da set gio RTC thanh cong: ");
        Serial.print(h); Serial.print(":"); Serial.println(m);
        updateClockFromRTC();
      } else {
        Serial.println("Gio hoac phut khong hop le (HH: 00-23, MM: 00-59)!");
      }
    }
  }
}