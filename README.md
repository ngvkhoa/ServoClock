# 7-Segment Mechanical Servo Clock (4 Digits)

Hệ thống đồng hồ cơ khí 4 chữ số 7 đoạn (HH:MM) hiển thị theo chuẩn 24 giờ, sử dụng **28 động cơ servo SG90/MG90S** điều khiển từng nét hiển thị riêng biệt. Dự án tích hợp module thời gian thực **RTC DS3231**, 2 mạch mở rộng **PCA9685 16-Channel PWM**, cùng còi **Buzzer** phát chuông kinh điển Westminster Quarters tại các mốc giờ cố định.

---

## 📌 Tính năng nổi bật

* **Hiển thị 24 giờ cơ khí sống động**: Mỗi nét trong chữ số 7 đoạn được nâng/gạt độc lập bởi 1 servo ($4 \times 7 = 28$ servo).
* **Trình tự chuyển động 3 bước chống kẹt (Anti-Collision Sequence)**: 
  * Thu gọn 6 nét ngoài $\rightarrow$ Định vị nét giữa `g` $\rightarrow$ Bung các nét ngoài theo số mới.
  * Triệt tiêu hoàn toàn nguy cơ va chạm cơ khí giữa nét `g` và các thanh xung quanh.
* **Tối ưu chuyển động thông minh (Diff Check)**: Chỉ xoay các servo có nét thay đổi, các số không đổi (ví dụ hàng chục giờ) được giữ nguyên vị trí, hạn chế sụt áp và tiếng ồn.
* **Thời gian thực chính xác**: Đồng bộ trực tiếp từ RTC DS3231 qua giao tiếp $I^2C$.
* **Chuông điểm giờ Westminster Quarters**: Phát giai điệu Big Ben qua Passive Buzzer tại các mốc `08:00`, `12:00` (12 tiếng chuông), và `18:30` (1 tiếng gõ giờ), tích hợp hiệu ứng "gõ búa kim loại" chống rè.
* **Giao diện UART/Terminal tiện dụng**: Hỗ trợ lệnh chỉnh giờ nhanh `SET HHMM` và kiểm tra chuông `p`.

---

## 🛠 Phần cứng & Sơ đồ đấu nối

### 1. Danh sách linh kiện
| Linh kiện | Số lượng | Địa chỉ $I^2C$ / Chân kết nối | Chức năng |
| :--- | :---: | :---: | :--- |
| **Vi điều khiển** (Arduino / ESP32) | 1 | — | Bộ xử lý trung tâm |
| **PCA9685 16-Channel PWM** | 2 | `0x40` (Board 1), `0x41` (Board 2) | Điều khiển 28 servo |
| **RTC DS3231** | 1 | `0x68` | Đồng hồ thời gian thực |
| **Servo SG90 / MG90S** | 28 | Cắm vào 2 board PCA9685 | Cơ cấu truyền động các nét |
| **Passive Buzzer** | 1 | Chân `D9` | Phát chuông điểm giờ |
| **Nguồn ngoài 5V – 6V (Tối thiểu 5A)** | 1 | Cọc domino PCA9685 | Cấp nguồn động lực cho servo |

---

### 2. Sơ đồ kênh PWM trên PCA9685

Mỗi chữ số sử dụng 7 kênh tương ứng với 7 nét `a, b, c, d, e, f, g`. Kênh 7 và 8 trên mỗi board được bỏ trống.


```

```
   [   Board 0x40 (GIỜ)   ]          [   Board 0x41 (PHÚT)  ]
 ┌─────────────┬─────────────┐     ┌─────────────┬─────────────┐
 │  Chục Giờ   │ Đơn vị Giờ  │     │  Chục Phút  │ Đơn vị Phút │
 │ (Kênh 9-15) │ (Kênh 0-6)  │     │ (Kênh 9-15) │ (Kênh 0-6)  │
 └─────────────┴─────────────┘     └─────────────┴─────────────┘

```

```

#### Chi tiết gán chân từng nét:
| Nét LED | Index | Kênh (Số Hàng Đơn Vị) | Kênh (Số Hàng Chục) | Góc Mở (`ON`) | Góc Đóng (`OFF`) |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **a** | 0 | Ch 0 | Ch 9 | 90° | 0° |
| **b** | 1 | Ch 1 | Ch 10 | 90° | 0° |
| **c** | 2 | Ch 2 | Ch 11 | 90° | 180° |
| **d** | 3 | Ch 3 | Ch 12 | 90° | 180° |
| **e** | 4 | Ch 4 | Ch 13 | 90° | 0° |
| **f** | 5 | Ch 5 | Ch 14 | 90° | 180° |
| **g** | 6 | Ch 6 | Ch 15 | 90° | 0° |

> **Lưu ý địa chỉ $I^2C$ của Board 2 (`0x41`)**: Hàn cầu chì / jumper **A0** trên module PCA9685 thứ hai để đổi địa chỉ từ `0x40` thành `0x41`.

---

## 💻 Cài đặt & Cấu hình phần mềm

### Thư viện Arduino cần cài đặt
Cài đặt trực tiếp qua **Arduino Library Manager**:
1. `Adafruit PWM Servo Driver Library`
2. `RTClib` (by Adafruit)

### Cấu hình nạp code
1. Kết nối vi điều khiển với máy tính qua cổng USB.
2. Mở file `.ino` bằng **Arduino IDE** hoặc **VS Code**.
3. Chọn đúng Board và Cổng COM.
4. Nạp code vào vi điều khiển.

---

## ⌨️ Hướng dẫn điều khiển qua Serial Monitor

Mở Serial Monitor với cấu hình:
* **Baudrate**: `115200`
* **Line ending**: `Newline` hoặc `Both NL & CR`

### Danh sách lệnh:
* `p` hoặc `P`: Chạy thử nhanh giai điệu chuông Westminster Quarters kèm 3 tiếng gõ giờ.
* `SET HHMM`: Cài đặt lại giờ cho RTC DS3231 trực tiếp mà không cần nạp lại code.
  * Ví dụ: `SET 1159` $\rightarrow$ Đặt giờ thành `11:59:00`.
  * Sau 1 phút nhảy sang `12:00:00`, đồng hồ sẽ kích hoạt chuông 12 tiếng.

---

## ⚠️ Lưu ý kỹ thuật & Vận hành

1. **Nguồn cấp cho Servo**:
   * Tuyệt đối **không** dùng chân 5V của Arduino/ESP32 để nuôi 28 servo. Dòng khởi động tức thời (stall current) có thể vượt $4\text{A} - 5\text{A}$ gây sập vi điều khiển.
   * Dùng nguồn tổ ong/adapter $5\text{V} - 6\text{V} \ge 5\text{A}$ cấp riêng vào cọc domino nguồn xanh trên 2 board PCA9685.
   * **Bắt buộc nối chung GND** giữa nguồn servo và vi điều khiển.
2. **Cân chỉnh cơ khí**:
   * Nếu có thanh nào bị tì cấn hoặc servo phát tiếng rít liên tục khi đóng/mở, điều chỉnh lại mảng `ANGLE_ON` và `ANGLE_OFF` tương ứng trong code để tránh nóng cháy motor.
