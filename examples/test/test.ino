#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <RTClib.h>

// --- CẤU HÌNH PHẦN CỨNG ---
#define BUZZER_PIN 9
#define SERVO_FREQ 50

// Khởi tạo RTC và 2 board PCA9685
RTC_DS3231 rtc;
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40); // 2 số GIỜ (bên trái)
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41); // 2 số PHÚT (bên phải)

// Xung PWM chuẩn cho SG90/MG90S ở 50Hz (chu kỳ 20ms)
const uint16_t PULSE_MIN = 102; // ~0 độ
const uint16_t PULSE_MAX = 512; // ~180 độ

// Cấu hình góc cho từng thanh: a, b, c, d, e, f, g (index 0 đến 6)
// Mở toàn bộ ở 90 độ. Đóng: c(2), d(3), f(5) ở 180 độ; các nét còn lại ở 0 độ.
const uint8_t ANGLE_ON[7]  = { 90,   90,  90,  90,  90,  90,  90};
const uint8_t ANGLE_OFF[7] = {  0,    0, 180, 180,   0, 180,   0};

// Kênh vật lý tương ứng 7 đoạn cho từng chữ số trên mỗi board
const uint8_t DIGIT_TENS_CH[7]  = {9, 10, 11, 12, 13, 14, 15}; // Hàng chục (bên trái)
const uint8_t DIGIT_UNITS_CH[7] = {0,  1,  2,  3,  4,  5,  6};  // Hàng đơn vị (bên phải)

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

// Biến quản lý thời gian
unsigned long lastRtcCheck = 0;
int lastAlarmMinute = -1; // Cờ lưu phút đã reo để tránh reo lặp lại trong cùng 1 phút

// --- CẤU HÌNH CHUÔNG WESTMINSTER ---
#define NOTE_B4   470   // Si 4
#define NOTE_E5   630   // Mi 5
#define NOTE_FS5  720   // Fa# 5
#define NOTE_GS5  810   // Sol# 5

const int chimeNotes[] = {
  NOTE_GS5, NOTE_FS5, NOTE_E5,  NOTE_B4,
  NOTE_E5,  NOTE_GS5, NOTE_FS5, NOTE_B4,
  NOTE_E5,  NOTE_FS5, NOTE_GS5, NOTE_E5,
  NOTE_GS5, NOTE_E5,  NOTE_FS5, NOTE_B4
};

const uint8_t noteBeats[] = {
  1, 1, 1, 2,
  1, 1, 1, 2,
  1, 1, 1, 2,
  1, 1, 1, 2
};

const int TEMPO = 430;

// ==========================================
// HÀM ĐIỀU KHIỂN CHUÔNG BUZZER
// ==========================================
void playBellTone(int freq, int durationMs) {
  // 1. Tiếng búa gõ kim loại đanh thép ở đầu nốt
  tone(BUZZER_PIN, freq * 2, 15);
  delay(15);

  // 2. Tiếng chuông chính ngân vang
  int bodyMs = durationMs * 0.78;
  tone(BUZZER_PIN, freq, bodyMs);
  delay(bodyMs);

  // 3. Dập tắt tiếng dứt khoát chống rè
  noTone(BUZZER_PIN);
  delay(durationMs - bodyMs);
}

void playWestminster(int strikes) {
  Serial.println(">>> DANG PHAT CHUONG WESTMINSTER <<<");

  // Phát 16 nốt chuông dạo
  for (int i = 0; i < 16; i++) {
    int duration = noteBeats[i] * TEMPO;
    playBellTone(chimeNotes[i], duration);

    if ((i + 1) % 4 == 0) {
      delay(220);
    }
  }

  delay(650); // Khoảng lặng trước khi điểm tiếng chuông giờ

  // Điểm tiếng chuông (Bong...)
  for (int strike = 0; strike < strikes; strike++) {
    tone(BUZZER_PIN, NOTE_E5 * 2, 25);
    delay(25);
    tone(BUZZER_PIN, NOTE_E5, 1200);
    delay(1200);
    noTone(BUZZER_PIN);
    delay(600);
  }
}

// ==========================================
// HÀM ĐIỀU KHIỂN SERVO HIỂN THỊ
// ==========================================
uint16_t angleToPulse(float degrees) {
  degrees = constrain(degrees, 0.0f, 180.0f);
  return (uint16_t)map(degrees, 0, 180, PULSE_MIN, PULSE_MAX);
}

uint8_t getCharPattern(char c) {
  if (c >= '0' && c <= '9') return NUM_MAP[c - '0'];
  if (c == '-') return 0b01000000;
  return 0b00000000;
}

void driveSegment(Adafruit_PWMServoDriver &board, const uint8_t* pinMap, uint8_t segIndex, bool state) {
  uint8_t angle = state ? ANGLE_ON[segIndex] : ANGLE_OFF[segIndex];
  board.setPWM(pinMap[segIndex], 0, angleToPulse(angle));
}

// Quy trình 3 bước an toàn: Thu ngoài -> Chỉnh g -> Bung ngoài
void updateDigit(uint8_t digitPos, char c) {
  if (currentDisplay[digitPos] == c) return; // Số không đổi thì giữ nguyên

  Adafruit_PWMServoDriver &board = (digitPos < 2) ? pwm1 : pwm2;
  const uint8_t* pinMap = (digitPos % 2 == 0) ? DIGIT_TENS_CH : DIGIT_UNITS_CH;

  uint8_t pattern = getCharPattern(c);

  // Bước 1: Dọn đường - Thu 6 nét ngoài về vị trí đóng
  for (uint8_t s = 0; s < 6; s++) {
    driveSegment(board, pinMap, s, false);
  }
  delay(70);

  // Bước 2: Chỉnh nét g
  bool state_g = (pattern >> 6) & 0x01;
  driveSegment(board, pinMap, 6, state_g);
  delay(70);

  // Bước 3: Dựng 6 nét ngoài theo số mới
  for (uint8_t s = 0; s < 6; s++) {
    bool state = (pattern >> s) & 0x01;
    driveSegment(board, pinMap, s, state);
  }

  currentDisplay[digitPos] = c;
}

void display4Digits(const char* str) {
  for (uint8_t i = 0; i < 4; i++) {
    updateDigit(i, str[i]);
  }
}

// ==========================================
// ĐỒNG BỘ THỜI GIAN VÀ BÁO GIỜ TỰ ĐỘNG
// ==========================================
void checkAndRunClock() {
  DateTime now = rtc.now();

  int h = now.hour();
  int m = now.minute();

  char timeStr[5];
  timeStr[0] = (h / 10) + '0';
  timeStr[1] = (h % 10) + '0';
  timeStr[2] = (m / 10) + '0';
  timeStr[3] = (m % 10) + '0';
  timeStr[4] = '\0';

  // 1. Cập nhật góc servo nếu có chữ số thay đổi
  if (timeStr[0] != currentDisplay[0] || timeStr[1] != currentDisplay[1] ||
      timeStr[2] != currentDisplay[2] || timeStr[3] != currentDisplay[3]) {
    
    Serial.print("[RTC 24H] ");
    Serial.print(timeStr[0]); Serial.print(timeStr[1]);
    Serial.print(":");
    Serial.print(timeStr[2]); Serial.println(timeStr[3]);

    display4Digits(timeStr);
  }

  // 2. Kiểm tra mốc chuông: 08:00, 12:00, 18:30
  // Chỉ kích hoạt 1 lần trong phút đó (khi lastAlarmMinute != m)
  if (lastAlarmMinute != m) {
    if (h == 8 && m == 0) {
      lastAlarmMinute = m;
      Serial.println("==> Den gio bao: 08:00!");
      playWestminster(8); // Báo 8 tiếng chuông
    }
    else if (h == 12 && m == 0) {
      lastAlarmMinute = m;
      Serial.println("==> Den gio bao: 12:00!");
      playWestminster(12); // Báo 12 tiếng chuông
    }
    else if (h == 18 && m == 30) {
      lastAlarmMinute = m;
      Serial.println("==> Den gio bao: 18:30!");
      playWestminster(1); // Mốc rưỡi điểm 1 tiếng chuông
    }
  }

  // Reset cờ reo khi đã bước sang phút khác
  if (m != 0 && m != 30) {
    lastAlarmMinute = -1;
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  pinMode(BUZZER_PIN, OUTPUT);

  // Khởi động RTC DS3231
  if (!rtc.begin()) {
    Serial.println("Loi: Khong tim thay RTC DS3231!");
  } else if (rtc.lostPower()) {
    Serial.println("RTC mat nguon, dat lai thoi gian bien dich.");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Khởi động 2 board PCA9685
  pwm1.begin();
  pwm1.setPWMFreq(SERVO_FREQ);

  pwm2.begin();
  pwm2.setPWMFreq(SERVO_FREQ);

  Serial.println("\n=== DONG HO 7 DOAN SERVO + CHUONG 24H SAN SANG ===");
  Serial.println("Cac moc bao chuong: 08:00, 12:00, 18:30");
  Serial.println("Lenh Serial:");
  Serial.println("  'p'       : Test nhanh bai chuong");
  Serial.println("  SET HHMM  : Chinh gio RTC (vd: SET 1829)");
  Serial.println("=================================================");

  // Đóng toàn bộ các nét lúc khởi động
  display4Digits("    ");
  delay(500);

  // Hiển thị giờ hiện tại ngay lập tức
  checkAndRunClock();
}

void loop() {
  // Quét thời gian mỗi 500ms
  if (millis() - lastRtcCheck >= 500) {
    lastRtcCheck = millis();
    checkAndRunClock();
  }

  // Xử lý lệnh từ Terminal
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "p" || input == "P") {
      playWestminster(3);
    }
    else if (input.startsWith("SET ") && input.length() == 8) {
      int h = input.substring(4, 6).toInt();
      int m = input.substring(6, 8).toInt();

      if (h >= 0 && h < 24 && m >= 0 && m < 60) {
        DateTime now = rtc.now();
        rtc.adjust(DateTime(now.year(), now.month(), now.day(), h, m, 0));
        Serial.print("Da cap nhat gio RTC: ");
        Serial.print(h); Serial.print(":"); Serial.println(m);
        lastAlarmMinute = -1; // Cho phép kiểm tra mốc chuông ngay
        checkAndRunClock();
      } else {
        Serial.println("Gio hoac phut khong hop le (HH: 00-23, MM: 00-59)!");
      }
    }
  }
}