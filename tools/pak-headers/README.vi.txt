Sora-WW — đọc thông tin footer PAK (chỉ đọc)

1. Giải nén ZIP vào thư mục riêng và chạy Inspect-Paks.cmd.
2. Trong cửa sổ chọn file, chọn BẤT KỲ file .pak nào thuộc Client\Content\Paks, đúng thư mục đã chụp ảnh. Có thể dán đường dẫn thư mục vào thanh địa chỉ của cửa sổ chọn file.
3. Sau khi xong, gửi file Sora-WW-Pak-Headers.json trên Desktop vào cuộc trò chuyện. Nếu có file trùng tên, bản mới có thêm mã ngắn ở tên.

Tiện ích tự liệt kê các .pak nằm trực tiếp trong cùng thư mục và chỉ đọc tối đa 4096 byte cuối mỗi file. Offset dùng số 64 bit nên vẫn làm việc với các gói trên 4 GB. Nó không đọc toàn bộ gói, không trích xuất/giải mã, không sửa/xóa PAK hoặc SIG, không đọc bộ nhớ game, không gửi phím/click và không gọi mạng. Chỉ tạo một báo cáo JSON mới trên Desktop. Báo cáo gồm tên gói, kích thước và footer mã hóa base64; không có đường dẫn cài đặt/tên tài khoản.

Đây là công cụ thu thập thông tin định dạng để xác định bước phân tích tiếp theo, chưa phải God Mode, No Cooldown hay Damage Multiplier. Không cần thay EXE/DLL của tool hiện tại và không cần gửi các gói PAK hàng GB.
