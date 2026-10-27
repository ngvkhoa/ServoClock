#define BUZZER_PIN 9

// Tần số chuẩn của tháp chuông Big Ben (Quãng 4 sạch tiếng, không rè)
#define NOTE_FS4 370   // Fa# 4
#define NOTE_GS4 415   // Sol# 4
#define NOTE_AS4 466   // La# 4
#define NOTE_CS5 554   // Đô# 5

// 16 nốt chuẩn giai điệu Westminster Quarters
const int chimeNotes[] = {
  // Câu 1: Fa#4 - La#4 - Sol#4 - Đô#5
  NOTE_FS4, NOTE_AS4, NOTE_GS4, NOTE_CS5,
  // Câu 2: Fa#4 - Sol#4 - La#4 - Fa#4
  NOTE_FS4, NOTE_GS4, NOTE_AS4, NOTE_FS4,
  // Câu 3: La#4 - Fa#4 - Sol#4 - Đô#5
  NOTE_AS4, NOTE_FS4, NOTE_GS4, NOTE_CS5,
  // Câu 4: Đô#5 - Sol#4 - La#4 - Fa#4
  NOTE_CS5, NOTE_GS4, NOTE_AS4, NOTE_FS4
};

// Phách nhịp: 1 = nốt ngắn, 2 = nốt ngân cuối câu
const uint8_t noteBeats[] = {
  1, 1, 1, 2,
  1, 1, 1, 2,
  1, 1, 1, 2,
  1, 1, 1, 2
};

const int TEMPO = 420; // ms mỗi phách

void playCleanTone(int freq, int durationMs) {
  // Dập tiếng sớm để màng loa dừng rung hẳn (chống rè nối âm)
  int soundMs = durationMs * 0.70; 
  int silenceMs = durationMs - soundMs;

  tone(BUZZER_PIN, freq, soundMs);
  delay(soundMs);
  noTone(BUZZER_PIN);
  delay(silenceMs);
}

void playWestminster() {
  Serial.println("Phat chuong Westminster (Tone Clean)...");

  for (int i = 0; i < 16; i++) {
    int duration = noteBeats[i] * TEMPO;
    playCleanTone(chimeNotes[i], duration);

    // Khoảng ngắt giữa các câu (mỗi 4 nốt)
    if ((i + 1) % 4 == 0) {
      delay(250);
    }
  }

  delay(600); // Khoảng lặng trước khi điểm tiếng chuông giờ

  // 3 tiếng chuông điểm giờ (Đô#5 ấm và ngân dứt khoát)
  for (int strike = 0; strike < 3; strike++) {
    playCleanTone(NOTE_CS5, 1000);
    delay(600);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  
  delay(1000);
  playWestminster();
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'p' || c == 'P') {
      playWestminster();
    }
  }
}