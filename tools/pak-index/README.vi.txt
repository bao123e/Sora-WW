Sora-WW — lấy vùng index của pakchunk43 (chỉ đọc)

1. Giải nén tiện ích vào thư mục riêng, chạy Inspect-Pak43.cmd.
2. Chọn Client\Content\Paks\pakchunk43-WindowsNoEditor.pak trong bản game đã gửi báo cáo header.
3. Gửi file Sora-WW-Pak43-Index.zip được tạo trên Desktop. Nếu có tên trùng, bản mới có thêm mã ngắn ở tên.

Báo cáo header đã xác minh 44 gói PAK phiên bản 12. Riêng pakchunk43 không đặt cờ mã hóa index; primary index dài 4.554.724 byte. Để tránh phải lấy dữ liệu nhiều lần, tiện ích lấy kèm vùng dữ liệu sau primary index đến trước footer: tổng vùng đọc là 17.382.089 byte (16,6 MiB), thay vì tải cả gói 690.064.639 byte. Dữ liệu này dùng để kiểm tra các mục lục và tên file; chưa khẳng định gói chứa gameplay script.

Tiện ích đối chiếu tên, kích thước và SHA256 footer với báo cáo bạn đã gửi; đọc vùng cho phép rồi kiểm tra SHA1 primary index đúng giá trị trong footer. Nếu bản game thay đổi hoặc hash không khớp thì dừng. Không giải mã, không trích xuất asset, không sửa/xóa file game/SIG, không đọc bộ nhớ game, không gửi click/phím và không gọi mạng. Chỉ tạo ZIP báo cáo mới trên Desktop, không đưa đường dẫn cài đặt hoặc thông tin tài khoản vào báo cáo. Đây chưa phải bản chức năng gameplay.
