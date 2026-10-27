#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <RTClib.h>

// Khởi tạo 2 board PCA9685 theo đúng địa chỉ quét được
Adafruit_PWMServoDriver pwm1 = Adafruit_PWMServoDriver(0x40); // 2 chữ số giờ (trái)
Adafruit_PWMServoDriver pwm2 = Adafruit_PWMServoDriver(0x41); // 2 chữ số phút (phải)

RTC_DS3231 rtc;

#define SERVO_FREQ 50

// Xung PWM chuẩn cho SG90/MG90S ở 50Hz (chu kỳ 20ms)
const uint16_t PULSE_MIN = 102; // ~0 độ
const uint16_t PULSE_MAX = 512; // ~180 độ

// Cấu hình góc cho từng thanh: a, b, c, d, e, f, g (index 0 đến 6)
// Nét 2 (c), 3 (d), và 5 (f) mở ở 180 độ; các nét khác mở ở 90 độ. Đóng toàn bộ ở 0 độ.
const uint8_t ANGLE_ON[7]  = { 90,  90, 180, 180,  90, 180,  90};
const uint8_t ANGLE_OFF[7] = {  0,   0,   0,   0,   0,   0,   0};

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

// Điều khiển một thanh cụ thể trên board và kênh chỉ định
void driveSegment(Adafruit_PWMServoDriver &board, const uint8_t* pinMap, uint8_t segIndex, bool state) {
  uint8_t angle = state ? ANGLE_ON[segIndex] : ANGLE_OFF[segIndex];
  board.setPWM(pinMap[segIndex], 0, angleToPulse(angle));
}

// Điều khiển 1 chữ số theo quy trình an toàn 3 bước: Reset ngoài -> Chỉnh g -> Dựng ngoài
void displayCharSafeSequence(Adafruit_PWMServoDriver &board, uint8_t digit, char c) {
  uint8_t pattern = getCharPattern(c);
  const uint8_t* pinMap = (digit == 0) ? DIGIT_TENS_CH : DIGIT_UNITS_CH;

  // BƯỚC 1: Dọn đường - Thu toàn bộ 6 nét ngoài (a -> f) về 0 độ
  for (uint8_t s = 0; s < 6; s++) {
    driveSegment(board, pinMap, s, false);
  }
  delay(60); // Đợi các nét ngoài dạt vào

  // BƯỚC 2: Chỉnh nét g (index 6) theo số mới
  bool state_g = (pattern >> 6) & 0x01;
  driveSegment(board, pinMap, 6, state_g);
  delay(60); // Đợi nét g cố định

  // BƯỚC 3: Dựng 6 nét ngoài (a -> f) theo đúng số mới
  for (uint8_t s = 0; s < 6; s++) {
    bool state = (pattern >> s) & 0x01;
    driveSegment(board, pinMap, pinMap == DIGIT_TENS_CH ? s : s, state); // sửa gọn tham số
    // Hoặc viết tường minh:
    // driveSegment(board, pinMap, s, state);
  }
}

// Hàm chuẩn điều khiển riêng cho Board 0x40 (2 số giờ) và Board 0x41 (2 số phút)
void displayDigitDirect(Adafruit_PWMServoDriver &board, uint8_t digitIndex, char c) {
  uint8_t pattern = getCharPattern(c);
  const uint8_t* pinMap = (digitIndex == 0) ? DIGIT_TENS_CH : DIGIT_UNITS_CH;

  // Bước 1: Thu nét ngoài
  for (uint8_t s = 0; s < 6; s++) {
    uint8_t angle = ANGLE_OFF[s];
    board.setPWM(pinMap[s], 0, angleToPulse(angle));
  }
  delay(50);

  // Bước 2: Chỉnh nét g
  bool state_g = (pattern >> 6) & 0x01;
  board.setPWM(pinMap[6], 0, angleToPulse(state_g ? ANGLE_ON[6] : ANGLE_OFF[6]));
  delay(50);

  // Bước 3: Mở nét ngoài
  for (uint8_t s = 0; s < 6; s++) {
    bool state = (pattern >> s) & 0x01;
    uint8_t angle = state ? ANGLE_ON[s] : ANGLE_OFF[s];
    board.setPWM(pinMap[s], 0, angleToPulse(angle));
  }
}

// Hiển thị toàn bộ 4 chữ số: HH:MM
void displayTime4Digits(char h0, char h1, char m0, char m1) {
  // Board 0x40 (Giờ)
  displayDigitDirect(pwm1, 0, h0); // Chục giờ
  delay(40);
  displayDigitDirect(pwm1, 1, h1); // Đơn vị giờ
  delay(40);

  // Board 0x41 (Phút)
  displayDigitDirect(pwm2, 0, m0); // Chục phút
  delay(40);
  displayDigitDirect(pwm2, 1, m1); // Đơn vị phút
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // Khởi động RTC
  if (!rtc.begin()) {
    Serial.println("Khong tim thay RTC DS3231!");
  } else if (rtc.lostPower()) {
    // Đặt lại thời gian biên dịch nếu RTC mất nguồn
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // Khởi động 2 board PCA9685
  pwm1.begin();
  pwm1.setPWMFreq(SERVO_FREQ);

  pwm2.begin();
  pwm2.setPWMFreq(SERVO_FREQ);

  Serial.println("Khoi dong dong ho 4 so (0x40 va 0x41) hoan tat!");

  // Đóng toàn bộ các nét lúc khởi động
  displayTime4Digits(' ', ' ', ' ', ' ');
  delay(1000);
}

void loop() {
  // Đọc thời gian thực từ RTC
  DateTime now = rtc.now();
  char h0 = (now.hour() / 10) + '0';
  char h1 = (now.hour() % 10) + '0';
  char m0 = (now.minute() / 10) + '0';
  char m1 = (now.minute() % 10) + '0';

  Serial.print("Thoi gian thuc: ");
  Serial.print(h0); Serial.print(h1);
  Serial.print(":");
  Serial.print(m0); Serial.println(m1);

  displayTime4Digits(h0, h1, m0, m1);

  // Cập nhật mỗi 1 giây (hoặc có thể tăng lên 10-30 giây để servo đỡ phải quét liên tục nếu phút chưa đổi)
  delay(1000); 
}