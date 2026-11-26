# Sora-WW — bản khôi phục 0.4.1

[Tải bản Windows khôi phục](https://github.com/bao123e/Sora-WW/releases/download/v0.4.1-recovery/WuWa-PC-0.4.1-recovery-win64.zip) · [Hướng dẫn](README.vi.md)

Chủ tool báo game đóng khi dùng gói thử 0.4. Bản vá đó đã tạm dừng; chưa xác định nguyên nhân gốc. Bản 0.4.1 đặt lại hai lựa chọn cũ, ngăn tạo lại gói và gỡ riêng gói có hash khớp hồ sơ của tool trước khi mở game.

Đóng game → Thoát tool cũ → giải nén toàn bộ ZIP mới (giữ cả hai EXE) → mở menu bằng phím đã lưu → **Khôi phục / Mở game**. PAK/SIG/EXE gốc không bị sửa. Gói không có hồ sơ hoặc đã thay đổi được giữ nguyên và báo lỗi.

Phím menu tùy chỉnh, ngôn ngữ, độ trong và đường dẫn EXE được giữ. Bật/tắt gameplay ngay trong phiên game chưa được triển khai; hai nút hiển thị Tạm dừng. Không coi trạng thái thấy cửa sổ game là gameplay đã hoạt động.

CI kiểm tra luồng menu → backend gỡ gói → mở EXE mô phỏng, đặt lại cấu hình cũ và chặn chuẩn bị gói mới. Cần người dùng xác nhận game đã vào lại sau hoàn tác. Không có game binary, DLL/PAK đầu ra, tài khoản hay khóa trong repository.

Build: `python scripts/build.py --zig /path/to/zig`. Đóng gói và kiểm tra Windows: xem `scripts/build_backend.py`, `scripts/verify_windows.py`, `scripts/package.py`. Source và bằng chứng kiểm tra được ghi trong từng bản phát hành.
