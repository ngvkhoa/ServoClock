#define BUZZER_PIN 9

// Tần số chuẩn Westminster cung E trưởng - Quãng 5 (To, vang, ấm)
#define NOTE_B4   470   // Si 4
#define NOTE_E5   630   // Mi 5
#define NOTE_FS5  720   // Fa# 5
#define NOTE_GS5  810   // Sol# 5

// 16 nốt chuẩn 100% giai điệu Westminster Quarters (Big Ben)
const int chimeNotes[] = {
  // Câu 1: Sol#5 - Fa#5 - Mi5 - Si4
  NOTE_GS5, NOTE_FS5, NOTE_E5,  NOTE_B4,
  // Câu 2: Mi5 - Sol#5 - Fa#5 - Si4
  NOTE_E5,  NOTE_GS5, NOTE_FS5, NOTE_B4,
  // Câu 3: Mi5 - Fa#5 - Sol#5 - Mi5
  NOTE_E5,  NOTE_FS5, NOTE_GS5, NOTE_E5,
  // Câu 4: Sol#5 - Mi5 - Fa#5 - Si4
  NOTE_GS5, NOTE_E5,  NOTE_FS5, NOTE_B4
};

// Phách: 1 = nốt ngắn, 2 = nốt ngân cuối câu
const uint8_t noteBeats[] = {
  1, 1, 1, 2,
  1, 1, 1, 2,
  1, 1, 1, 2,
  1, 1, 1, 2
};

const int TEMPO = 430; // Tốc độ nhịp chuông (ms/phách)

// Hàm tạo hiệu ứng tiếng chuông gõ kim loại (Attack + Ring)
void playBellTone(int freq, int durationMs) {
  // 1. Tiếng "búa gõ" kim loại đanh thép ở đầu nốt (15ms hài bậc cao)
  tone(BUZZER_PIN, freq * 2, 15);
  delay(15);

  // 2. Tiếng chuông chính ngân vang
  int bodyMs = durationMs * 0.78;
  tone(BUZZER_PIN, freq, bodyMs);
  delay(bodyMs);

  // 3. Dập tắt tiếng dứt khoát để ngắt nốt rõ ràng, chống rè
  noTone(BUZZER_PIN);
  delay(durationMs - bodyMs);
}

void playWestminster() {
  Serial.println("Phat chuong Westminster Quarters (Tone E5)...");

  for (int i = 0; i < 16; i++) {
    int duration = noteBeats[i] * TEMPO;
    playBellTone(chimeNotes[i], duration);

    // Nghỉ nhẹ 220ms sau mỗi câu (4 nốt)
    if ((i + 1) % 4 == 0) {
      delay(220);
    }
  }

  delay(650); // Khoảng lặng trước khi điểm tiếng chuông giờ (Bong...)

  // 3 tiếng chuông điểm giờ mẫu (Mi trầm ngân vang)
  Serial.println("Diem gio: Bong... Bong... Bong...");
  for (int strike = 0; strike < 3; strike++) {
    // Tiếng búa gõ đanh đầu tiếng chuông giờ
    tone(BUZZER_PIN, NOTE_E5 * 2, 25);
    delay(25);
    // Ngân tiếng chuông chính
    tone(BUZZER_PIN, NOTE_E5, 1200);
    delay(1200);
    noTone(BUZZER_PIN);
    delay(600); // Giãn cách giữa các tiếng gõ
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);

  delay(1000);
  playWestminster();
  Serial.println("Nhan 'p' tren Serial de nghe lai.");
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'p' || c == 'P') {
      playWestminster();
    }
  }
}