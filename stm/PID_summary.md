# PID trong dự án STM

1. **Có tồn tại PID**
   - Các hàm PID được khai báo trong `BALANCE/control.h`:
     - `int Position_PID_A (int Encoder,int Target);`
     - `int Position_PID_B (int Encoder,int Target);`
     - `int Position_PID_C (int Encoder,int Target);`
     - `int Position_PID_D (int Encoder,int Target);`
   - Các hàm PI tăng dần (Incremental_PI) cũng có trong cùng file.

2. **Bộ tham số robot liên quan tới PID**
   - Trong `HARDWARE/stmflash.c` có mảng `PID_Parameter[10]` lưu các giá trị:
     - `PID_Parameter[0]` → `RC_Velocity` (tốc độ mục tiêu)
     - `PID_Parameter[1]` → `Velocity_KP` (hệ số Kp)
     - `PID_Parameter[2]` → `Velocity_KI` (hệ số Ki)
     - Các phần tử còn lại được dự trữ cho các tham số mở rộng.
   - Khi flash chưa có dữ liệu (giá trị 0xFFFF) chương trình sẽ dùng giá trị mặc định:
     `RC_Velocity = 30; Velocity_KP = 12; Velocity_KI = 12;`

3. **Hàm truyền/đọc tham số PID**
   - `void Flash_Read(void)`: đọc 10 từ flash vào `PID_Parameter`, sau đó gán vào `RC_Velocity`, `Velocity_KP`, `Velocity_KI`.
   - `void Flash_Write(void)`: ghi các giá trị hiện tại của `RC_Velocity`, `Velocity_KP`, `Velocity_KI` (và các phần còn lại) vào flash.
   - Cờ `PID_Send` (định nghĩa ở `BALANCE/system.c` & `BALANCE/system.h`) được đặt thành `1` ở ISR UART khi nhận lệnh `0x50`. Khi `PID_Send==1` các hàm truyền dữ liệu PID sẽ được thực hiện (chi tiết thực thi nằm trong `usartx.c`).

4. **Tóm tắt**
   - Dự án có bộ điều khiển PID cho 4 bánh (A‑D) được khai báo trong `control.h`.
   - Các tham số PID được lưu trong flash dưới dạng mảng `PID_Parameter` và được đọc/ghi bằng `Flash_Read/Flash_Write`.
   - Cờ `PID_Send` và hàm UART trong `usartx.c` chịu trách nhiệm truyền các giá trị PID ra bên ngoài (ví dụ: tới PC hoặc thiết bị điều khiển).

File này cung cấp cái nhìn nhanh về việc sử dụng PID, các tham số lưu trữ và cách truyền/đọc chúng.
