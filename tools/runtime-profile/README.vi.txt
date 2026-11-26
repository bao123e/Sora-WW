Sora WW - Runtime inspection v2

1. Mở Wuthering Waves và vào game, chờ tải xong. Có thể Alt+Tab để chạy tiện ích.
2. Giải nén ZIP vào thư mục riêng, chạy Inspect-Runtime.cmd.
3. Gửi file Sora-WW-Runtime-xxxxxxxx.json mới tạo trên Desktop.
Nếu báo STOPPED, gửi nội dung lỗi; không cần tải thêm PAK hay DLL.

Tiện ích chỉ lấy danh sách tên/phiên bản module do Windows cung cấp và hash SHA256 của EXE trên đĩa để đối chiếu file đã gửi. Không ghi địa chỉ bộ nhớ, đường dẫn cài đặt, tên tài khoản, ảnh màn hình hay dữ liệu tài khoản vào báo cáo. Không sửa bộ nhớ/file game, không gửi phím/click, không gọi mạng, không cài dịch vụ hoặc driver.

Đây là kiểm tra môi trường chạy, chưa phải God Mode/No Cooldown/Damage Multiplier. Tên DLL chỉ là dấu hiệu; runtime có thể được liên kết trực tiếp vào EXE nên không thấy DLL không đồng nghĩa không có runtime đó. Việc lấy danh sách module có thể bị Windows hoặc game từ chối; tiện ích sẽ dừng và không tìm cách vượt hạn chế.

Bản v2 không dừng nếu ProductName trống hoặc khác. Nhận diện MATCHES_UPLOADED_EXE chỉ dựa trên SHA256 trùng EXE đã gửi; nếu khác, báo cáo vẫn được tạo với trạng thái DIFFERENT_FROM_UPLOADED_EXE để phân tích, không coi đó là bản được hỗ trợ.
