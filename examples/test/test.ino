#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

#define SERVO_FREQ 50

// Xung PWM chuẩn cho SG90/MG90S ở 50Hz (chu kỳ 20ms)
const uint16_t PULSE_MIN = 102; // ~0 độ
const uint16_t PULSE_MAX = 512; // ~180 độ

// Cấu hình góc cho từng thanh: a, b, c, d, e, f, g
// Index:  0=a,  1=b,  2=c,  3=d,  4=e,  5=f,  6=g
// Nét 2 (c) và 5 (f) mở ở 180 độ; các nét còn lại mở ở 90 độ. Đóng toàn bộ ở 0 độ.
const uint8_t ANGLE_ON[7]  = { 90,  90,  90,  90,  90, 90,  90};
const uint8_t ANGLE_OFF[7] = {  0,   0,   180,   180,   0,   180,   0};

// Kênh vật lý tương ứng 7 đoạn cho từng chữ số trên board 0x40
// HÀNG CHỤC (Chữ số bên trái): kênh 9 - 15
const uint8_t DIGIT_TENS_CH[7]  = {9, 10, 11, 12, 13, 14, 15};
// HÀNG ĐƠN VỊ (Chữ số bên phải): kênh 0 - 6
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

// Điều khiển một thanh cụ thể (segIndex: 0=a, 1=b, ..., 6=g)
void driveSegment(const uint8_t* pinMap, uint8_t segIndex, bool state) {
  uint8_t angle = state ? ANGLE_ON[segIndex] : ANGLE_OFF[segIndex];
  pwm.setPWM(pinMap[segIndex], 0, angleToPulse(angle));
}

// Điều khiển chữ số theo quy trình: Reset ngoài -> Chỉnh g -> Dựng ngoài
// digit: 0 là Hàng chục (9-15), 1 là Hàng đơn vị (0-6)
void displayCharSafeSequence(uint8_t digit, char c) {
  uint8_t pattern = getCharPattern(c);
  const uint8_t* pinMap = (digit == 0) ? DIGIT_TENS_CH : DIGIT_UNITS_CH;

  // BƯỚC 1: Dọn đường - Thu toàn bộ 6 nét ngoài (a -> f) về vị trí đóng (0 độ)
  for (uint8_t s = 0; s < 6; s++) {
    driveSegment(pinMap, s, false);
  }
  delay(80); // Đợi các nét ngoài dạt hẳn vào trong

  // BƯỚC 2: Đưa nét g (index 6) về trạng thái đích của số mới
  bool state_g = (pattern >> 6) & 0x01;
  driveSegment(pinMap, 6, state_g);
  delay(80); // Đợi nét g cố định vị trí

  // BƯỚC 3: Dựng 6 nét ngoài (a -> f) theo đúng số mới
  for (uint8_t s = 0; s < 6; s++) {
    bool state = (pattern >> s) & 0x01;
    driveSegment(pinMap, s, state);
  }
}

// Hiển thị chuỗi 2 ký tự (chục, đơn vị)
void displayString(const char* str) {
  displayCharSafeSequence(0, str[0]); // Chữ số hàng chục (kênh 9-15)
  displayCharSafeSequence(1, str[1]); // Chữ số hàng đơn vị (kênh 0-6)
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);

  Serial.println("Khoi dong: Hang chuc (9-15), Hang don vi (0-6).");
  Serial.println("Cau hinh goc: Net 2 va 5 mo o 180 do, cac net khac mo o 90 do. Tat ca dong o 0 do.");

  // Đóng toàn bộ về trạng thái ban đầu
  displayString("  ");
  delay(1000);
}

void loop() {
  // Test dem so tu 00 den 99
  for (int i = 0; i < 100; i++) {
    char buf[3];
    sprintf(buf, "%02d", i);
    Serial.print("Hien thi: "); Serial.println(buf);
    
    displayString(buf);
    delay(1200);
  }
}