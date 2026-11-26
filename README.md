# Sora-WW — WuWa PC

Khung tool Windows dành cho Wuthering Waves, phát triển từng bước và xác minh riêng từng phần.

**[Tải bản khung Windows](https://github.com/bao123e/Sora-WW/releases/download/v0.2.0-ui-preview/WuWa-PC-0.2.0-ui-preview-win64.zip)** · [Releases](https://github.com/bao123e/Sora-WW/releases) · [Hướng dẫn đầy đủ](README.vi.md)

Giao diện 0.2 dùng nền tối trong nhẹ, thanh tiêu đề xanh, tab Tổng quan / Phím tắt và thanh đổi phím/ngôn ngữ ở cuối theo ảnh tham khảo của chủ tool. Có Ghim, cuộn nội dung, độ đậm menu và tiếng Việt/English được lưu lại.

Bước 1 hiện có menu mặc định ẩn, căn giữa cửa sổ game, phím mở menu tùy chỉnh và lưu lại, biểu tượng khay hệ thống, nhận diện cửa sổ/tiến trình ở chế độ chỉ đọc. **Chưa có chức năng gameplay.**

Giải nén toàn bộ ZIP → chạy `Start-WuWa-PC.cmd` → nhấn **F1** mở menu. Đổi phím bằng ô phím và **Lưu**. X chỉ ẩn; **Thoát** thoát hẳn. Game nên ở chế độ cửa sổ hoặc không viền để thử menu.

Ảnh do chủ tool cung cấp ngày 26/11/2026 hiển thị `OS_Windows_Product_3.8.0_9244589_3.8.13_9534983 * SD`, khu vực SEA; launcher 3.1.0.1. Đây là thông tin quan sát, chưa phải xác nhận tương thích gameplay hay phiên bản engine.

## Kiểm thử và phát hành

GitHub Actions build bằng Zig 0.14.1, kiểm tra PE x64, chạy các test logic và runtime Windows. Native test dùng cửa sổ giả lập; test EXE kiểm tra khởi động ẩn, mở/ẩn/thoát, lưu phím và nạp lại qua lần chạy tiếp theo. Không chạy game hay gửi bàn phím/chuột trong CI. Chỉ tạo Release nếu các bước đạt; `build.json` ghi hash EXE, bằng chứng kiểm thử và `game_runtime_tested: false`.

Local Linux: `python scripts/test.py` và `python scripts/build.py --zig /path/to/zig`. Trên Windows: `python scripts/verify_windows.py --zig zig`, rồi `python scripts/package.py`.

Dự án này độc lập với SoraVynx. Không chứa issuer/key/token của tool cũ, không sửa bộ nhớ game, tiêm DLL hay gọi mạng. Giai đoạn tiếp theo sẽ dựa trên kết quả kiểm tra thực tế, không suy diễn từ việc build thành công.
