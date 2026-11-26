# Sora WW — No Cooldown trong phiên game, bản alpha

Bản này triển khai No Cooldown bằng phần xử lý trong phiên đang chạy. Không dùng lại cơ chế sửa gói 0.4 hoặc yêu cầu bật ở lần mở game sau. Chặn sát thương và Damage Multiplier chưa có bản triển khai trong phiên game.

1. Thoát tool cũ. Giải nén **toàn bộ** ZIP mới vào thư mục riêng; giữ ba EXE và ba DLL cạnh nhau.
2. Vào game bình thường. Mở tool và menu bằng phím tùy chỉnh đã lưu. Chọn đúng Client\Binaries\Win64\Client-Win64-Shipping.exe nếu chưa lưu đường dẫn.
3. Bấm **Bật trong game** cạnh No Cooldown. Chỉ khi backend xác nhận đã gắn hook, menu mới hiện nút **Tắt** và bộ đếm `checks:starts`.
4. Thử kỹ năng E. Bộ đếm cho biết hàm đã bị chặn bao nhiêu lần; chưa chứng minh cooldown thực tế hoặc kiểm tra phía server đã thay đổi.
5. Bấm **Tắt** để dùng lại xử lý gốc. Thoát tool hoặc mất kết nối cũng tự vô hiệu hóa trong tối đa ba giây. Ẩn menu vẫn giữ lựa chọn trong phiên hiện tại; lần chạy sau luôn bắt đầu tắt.

Bản alpha đã có mã chức năng và kiểm thử cơ chế trên chương trình mô phỏng, **chưa được xác nhận hoạt động với Mono nhúng/game thật của bạn**. Nếu thấy mã lỗi, chức năng chưa được bật. Không yêu cầu bạn tìm thêm PAK, DLL hoặc chạy công cụ Inspect. Không cố vượt lỗi quyền truy cập, tải runtime thay thế hoặc đoán địa chỉ hàm.

Tool nạp DLL do dự án biên dịch vào tiến trình đã chọn, dùng API Mono công khai đang có sẵn rồi gắn prefix bằng Harmony. Nó kiểm tra MVID, token, chữ ký và hash IL gốc trước khi thay đổi. Chỉ nhân vật thuộc người chơi hiện tại được can thiệp; nhân vật khác, đối tượng đã hủy hoặc không xác minh được chủ sở hữu dùng hàm gốc. DLL/PAK/SIG/EXE gốc không bị sửa. Hook đã tắt và bridge được giữ đến khi game thoát để tránh tháo DLL không an toàn.

Nếu còn gói thử 0.4, đóng game và dùng **Khôi phục / Mở game** trước. Backend chỉ gỡ đúng gói có hash khớp hồ sơ tool. Không xóa PAK gốc.

Giao diện, căn giữa, Ghim, độ trong, ngôn ngữ và phím menu tùy chỉnh được giữ. Nút × chỉ ẩn; nút **Thoát** đóng ứng dụng.

Kiểm thử gồm các detour Harmony thật trên hàm mô phỏng (bật/tắt/bật lại, lọc chủ sở hữu, mất kết nối), cùng nạp bridge và pipe Windows trên EXE mô phỏng của dự án. Không coi những kiểm thử này là xác nhận hiệu lực gameplay. `build.json` ghi rõ `game_runtime_tested: false`.
