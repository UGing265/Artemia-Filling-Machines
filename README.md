# Máy Chiết Artemia

Máy chiết chiết tự động cho artemia (tôm muối) với kiểm soát thể tích chính xác và giám sát thời gian thực.

## Vấn Đề

Chiết chiết ống tube thủ công cho artemia tốn thời gian và không nhất quán. Mỗi ống cần liều lượng chính xác 5ml với xác nhận trực quan, và việc theo dõi số lượng ống trong quá trình xử lý là rất quan trọng để kiểm soát chất lượng.

## Giải Pháp

Hệ thống chiết chiết tự động dựa trên Arduino:
- Chiết ống với thể tích 5ml tự động
- Đếm số ống đã chiết bằng cảm biến quang điện
- Hiển thị tốc độ, thể tích và số lượng thời gian thực trên LCD
- Cho phép hiệu chỉnh tốc độ dòng chảy qua biến trở
- Hỗ trợ hủy/đặt lại bằng nút bấm giữ lâu

## Linh Kiện

| Linh Kiện | Công Dụng |
|-----------|-----------|
| Arduino CNC Shield V3.00 | Board điều khiển chính |
| L298N DC Motor Driver | Điều khiển tốc độ/chiều motor bơm |
| Nema 17 Stepper Motor | Truyền động cơ cấu xoay |
| LCD 16x4 I2C (PCF8574) | Hiển thị trạng thái và số liệu |
| Cảm Biến Quang (E3F-DS30C4) | Phát hiện vị trí ống để đếm |
| Công Tắc Hành Trình (JL012-13) | Nút hủy/đặt lại |
| Biến Trở 10K | Điều chỉnh tốc độ bơm |
| Bơm Peristaltic 5V | Chiết dung dịch artemia |

## Máy Trạng Thái

```
IDLE → [giữ 3s] → CALIBRATING → [5s] → CALIB_INPUT → [lưu] → IDLE
     → [tự động] → PUMPING → [5ml HOẶC hủy] → IDLE
```

## Chế Độ Hiệu Chỉnh

1. Giữ nút reset 3 giây ở trạng thái IDLE
2. Bơm chạy 5 giây ở tốc độ tối đa
3. Dùng biến trở đặt thể tích đã đo
4. Nhấn ngắn để lưu, nhấn dài để hủy

## Demo

<!-- Video demo sẽ được thêm sau -->

## Nhóm

- **kiến trúc sư**: ZT
- **kĩ sư**: DevShiroru