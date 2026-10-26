#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(9600);

  while (!Serial);

  Serial.println("================================");
  Serial.println("       I2C SCANNER");
  Serial.println("================================");
  Serial.println("Dang quet dia chi I2C...");
}

void loop() {
  byte error;
  byte address;
  int deviceCount = 0;

  Serial.println("\nBat dau quet I2C:");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("Tim thay thiet bi I2C tai dia chi: 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);

      deviceCount++;
    }
  }

  Serial.println("--------------------------------");

  if (deviceCount == 0) {
    Serial.println("Khong tim thay thiet bi I2C!");
  } else {
    Serial.print("Tong so thiet bi tim thay: ");
    Serial.println(deviceCount);
  }

  Serial.println("Quet lai sau 3 giay...\n");
  delay(3000);
}
