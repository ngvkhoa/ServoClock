# 7-Segment Mechanical Servo Clock (4 Digits)

Hệ thống đồng hồ cơ khí 4 chữ số 7 đoạn (HH:MM) hiển thị theo chuẩn 24 giờ, sử dụng **động cơ servo SG90/MG90S** điều khiển từng nét hiển thị riêng biệt. Dự án tích hợp module thời gian thực **RTC DS3231**, 2 mạch mở rộng **PCA9685 16-Channel PWM**, cùng còi **Buzzer** phát chuông kinh điển Westminster Quarters tại các mốc giờ cố định.

---

## ⚠️ LƯU Ý PHẦN CỨNG QUAN TRỌNG (HARDWARE NOTE)

> **Kênh 14 trên Board `0x40` (Nét `f` - Hàng chục giờ) đã được rút/ngắt kết nối do servo bị hỏng cơ học.**
> * **Hiện trạng**: Hàng chục giờ hiển thị các số từ `0` đến `2` (chuẩn 24H: `00` – `23`):
>   * Số `0`: Thiếu nét `f` (bên trái trên) nhưng vẫn nhận diện được dạng chữ số.
>   * Số `1`: Không sử dụng nét `f` $\rightarrow$ Hiển thị bình thường.
>   * Số `2`: Không sử dụng nét `f` $\rightarrow$ Hiển thị bình thường.
> * **Khi thay thế/sửa chữa**: Sau khi thay servo mới vào vị trí nét `f` hàng chục giờ, chỉ cần cắm lại dây vào **Kênh 14 của Board `0x40`**, code tự động điều khiển lại bình thường mà không cần sửa firmware.

---

## 📌 Tính năng nổi bật

* **Hiển thị 24 giờ cơ khí sống động**: Mỗi nét trong chữ số 7 đoạn được nâng/gạt độc lập bởi 1 servo.
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
| **PCA9685 16-Channel PWM** | 2 | `0x40` (Board 1), `0x41` (Board 2) | Điều khiển các servo |
| **RTC DS3231** | 1 | `0x68` | Đồng hồ thời gian thực |
| **Servo SG90 / MG90S** | 27 (gốc 28) | Cắm vào 2 board PCA9685 | Cơ cấu truyền động các nét |
| **Passive Buzzer** | 1 | Chân `D9` | Phát chuông điểm giờ |
| **Nguồn ngoài 5V – 6V (Tối thiểu 5A)** | 1 | Cọc domino PCA9685 | Cấp nguồn động lực cho servo |

---

### 2. Sơ đồ kênh PWM trên PCA9685

Mỗi chữ số sử dụng 7 kênh tương ứng với 7 nét `a, b, c, d, e, f, g`. Kênh 7 và 8 trên mỗi board được bỏ trống.
