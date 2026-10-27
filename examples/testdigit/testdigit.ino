#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x41); // sửa địa chỉ ở đây để điều khiển

#define SERVO_FREQ 50

// Giới hạn xung PWM tương ứng tần số 50Hz (chu kỳ 20ms, đếm 0 - 4095)
// SG90/MG90S thường nhận xung 0.5ms - 2.5ms
const uint16_t PULSE_MIN = 102; // ~0 độ
const uint16_t PULSE_MAX = 512; // ~180 độ

uint8_t currentChannel = 0;

void setServoAngle(uint8_t ch, float degrees) {
  degrees = constrain(degrees, 0.0f, 180.0f);
  uint16_t pulse = (uint16_t)map(degrees, 0, 180, PULSE_MIN, PULSE_MAX);
  pwm.setPWM(ch, 0, pulse);

  Serial.print("Kenh "); Serial.print(ch);
  Serial.print(" -> Goc: "); Serial.print((int)degrees);
  Serial.print(" deg (Pulse: "); Serial.print(pulse); Serial.println(")");
}

void setServoPulse(uint8_t ch, uint16_t pulse) {
  pulse = constrain(pulse, 80, 600);
  pwm.setPWM(ch, 0, pulse);

  Serial.print("Kenh "); Serial.print(ch);
  Serial.print(" -> Raw Pulse: "); Serial.println(pulse);
}

void printHelp() {
  Serial.println("\n=== TEST BOARD 0x40 (KENH 0-6 VA 9-15) ===");
  Serial.println("  ch <0-6 | 9-15> : Chon kenh servo can test");
  Serial.println("  deg <0-180>     : Quay den goc do");
  Serial.println("  pulse <100-500> : Set truc tiep tick PWM de can chinh");
  Serial.println("  sweep           : Quet goc 0 -> 180 -> 0 tren kenh dang chon");
  Serial.println("  all <deg>       : Set tat ca 14 servo ve cung mot goc");
  Serial.println("===========================================");
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);

  printHelp();
  Serial.println("Dang chon mac dinh: Kenh 0.");
}

void loop() {
  if (Serial.available() > 0) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.length() == 0) return;

    if (cmd.startsWith("ch ")) {
      int ch = cmd.substring(3).toInt();
      if ((ch >= 0 && ch <= 6) || (ch >= 9 && ch <= 15)) {
        currentChannel = ch;
        Serial.print("Da chon Kenh: "); Serial.println(currentChannel);
      } else {
        Serial.println("Luu y: Chi dung kenh 0-6 va 9-15 (kenh 7, 8 khong dung)!");
      }
    } 
    else if (cmd.startsWith("deg ")) {
      float deg = cmd.substring(4).toFloat();
      setServoAngle(currentChannel, deg);
    } 
    else if (cmd.startsWith("pulse ")) {
      int p = cmd.substring(6).toInt();
      setServoPulse(currentChannel, p);
    } 
    else if (cmd.equals("sweep")) {
      Serial.println("Dang quet 0 -> 180 do...");
      for (int a = 0; a <= 180; a += 10) {
        setServoAngle(currentChannel, a);
        delay(60);
      }
      for (int a = 180; a >= 0; a -= 10) {
        setServoAngle(currentChannel, a);
        delay(60);
      }
      Serial.println("Hoan tat sweep.");
    } 
    else if (cmd.startsWith("all ")) {
      float deg = cmd.substring(4).toFloat();
      Serial.print("Dang set tat ca 14 kenh ve: "); Serial.println(deg);
      
      const uint8_t activeChannels[] = {0, 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15};
      for (uint8_t ch : activeChannels) {
        setServoAngle(ch, deg);
        delay(35); // Tranh sut ap nguon dot ngot khi 14 servo cung an dong
      }
    } 
    else {
      Serial.print("Khong ro lenh: "); Serial.println(cmd);
      printHelp();
    }
  }
}