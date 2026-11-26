Sora WW - đọc riêng assembly gameplay v2

1. Giải nén vào thư mục riêng, chạy Read-Gameplay.cmd.
2. Chọn Client\Content\Paks\pakchunk18-WindowsNoEditor.pak trong bản cài đặt đã gửi báo cáo trước đó.
3. Nếu thành công, gửi Sora-WW-AkiGame-xxxxxxxx.zip trên Desktop vào cuộc trò chuyện. Không đăng ZIP này lên GitHub vì chứa DLL của game.
Nếu STOPPED, gửi đúng dòng lỗi; không cần gửi cả gói PAK.

Tiện ích đọc gói khoảng 100 MB một lần và xác minh kích thước/hash footer theo dữ liệu đã nhận. Nó tìm PE trong các entry không nén, không mã hóa; xác minh SHA1 entry, định dạng managed PE và tên assembly Aki.Game. Bỏ 11 byte zero đứng trước PE để tạo DLL có thể phân tích offline. Đọc AssemblyName chỉ đọc metadata, không nạp/chạy code trong DLL. Các file ứng viên tạm được xóa sau khi đọc. Chỉ tạo ZIP mới trên Desktop, không thay file game, không tiêm DLL, không đọc tiến trình game, không gọi mạng, không yêu cầu khóa giải mã.

Đây là bước lấy đúng đầu vào phục vụ phân tích gameplay. Nếu dữ liệu thực tế khác mô tả plain-entry, tiện ích dừng; không coi việc đọc thành công là xác nhận ba chức năng gameplay đã hoạt động. ZIP gồm Aki.Game.dll và báo cáo hash; không có đường dẫn cài đặt hoặc thông tin tài khoản.

Bản v2 sửa theo gói thật đã nhận: kích thước và SHA1 trong header tính từ MZ, không tính 11 byte căn lề trước PE.
