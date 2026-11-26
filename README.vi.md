# WuWa PC — menu và mở game

Bản 0.3.1-launcher-preview thêm chọn/lưu EXE và mở game từ menu. Chưa có God Mode, Damage, cooldown, năng lượng, hút quái hoặc auto nhiệm vụ. Không có thông báo “gameplay READY”.

Thông tin chủ tool đã cung cấp ngày 26/11/2026:

- Launcher chính thức: 3.1.0.1.
- Màn hình game: `OS_Windows_Product_3.8.0_9244589_3.8.13_9534983 * SD`.
- Khu vực SEA.
- EXE đã gửi có tài nguyên phiên bản UE4 4.26; chưa xác minh API đồ họa khi game chạy. Chuỗi trên là bằng chứng từ ảnh màn hình, không phải chứng nhận tương thích executable.

## Dùng thử

1. Giải nén toàn bộ gói vào một thư mục riêng. Để game ở chế độ cửa sổ hoặc không viền để dùng menu.
2. Chạy `Start-WuWa-PC.cmd`. Trình khởi động tự kiểm tra control Windows và lưu/đọc cấu hình tạm trước khi mở chương trình. Không cần gửi file game lớn.
3. Menu mặc định ẩn. Nhấn **F1**, hoặc nhấp biểu tượng WuWa PC dưới khay hệ thống để mở.
4. Trong Tổng quan, chọn **Chọn EXE…**, tới `Client\Binaries\Win64\Client-Win64-Shipping.exe` của bản cài đặt. Đường dẫn được lưu riêng cùng cấu hình, hỗ trợ thư mục có dấu. Bấm **Mở game** để mở EXE đã chọn với tham số `-krqlv=sd` do chủ tool cung cấp cho bản SD. Không cần thêm tham số vào tên file hoặc tạo shortcut. Nếu game yêu cầu launcher chính thức, dùng launcher đó; tool vẫn nhận diện cửa sổ của EXE đã chọn. Menu chỉ tự ẩn sau khi thấy cửa sổ game, không dựa vào thời gian tải cố định. Bấm mở nhiều lần sẽ không mở thêm phiên khi tiến trình game đang chạy.
5. Bấm ô phím, nhấn tổ hợp muốn dùng, rồi **Lưu**. Hỗ trợ phím riêng hoặc Ctrl/Alt/Shift. **Mặc định** chỉ điền lại F1; cần Lưu để áp dụng. Phím trùng hoặc không lưu được cấu hình sẽ giữ lựa chọn cũ.
6. X ở góc cửa sổ chỉ ẩn menu. **Thoát** trong menu hoặc menu chuột phải của biểu tượng khay hệ thống sẽ thoát hẳn và bỏ đăng ký phím.

Phím được lưu tại `%LOCALAPPDATA%\WuWaPC\settings.ini`, riêng cho sản phẩm này. Không dùng key, issuer hoặc cấu hình thanh toán của SoraVynx.

## Giao diện theo ảnh tham khảo

Nền tối có độ trong nhẹ, thanh tiêu đề xanh, tab Tổng quan / Phím tắt, nhóm có đường phân cách và thanh đổi phím/ngôn ngữ luôn ở cuối. Tab Tổng quan có nút chọn độ đậm menu (85/90/94/100%). Ghim giữ menu ở trên các cửa sổ khác. Chuyển tiếng Việt / English ở thanh cuối. Những tùy chọn này được lưu cùng cấu hình phím; phím đã chọn ở bản 0.1 vẫn dùng được.

Đổi phím ở tab Phím tắt hoặc nút Đổi phím phía dưới. Cuộn con lăn hoặc kéo thanh cuộn khi nội dung dài. Ảnh tham khảo chỉ được dùng cho giao diện; không có tùy chọn Frame Generation, DLSS hay GPU giả lập.

## Cần kiểm tra ở lượt đầu

- Menu có bắt đầu ẩn và mở bằng phím đã chọn không?
- Menu có nằm giữa cửa sổ game không?
- Đổi phím, thoát hẳn, mở lại: phím có được giữ không?
- Dòng trạng thái có hiện **Game window found (read only)** không? Nếu chưa, gửi đúng dòng trạng thái; chưa cần gửi đường dẫn hoặc file game.

Bộ dò chỉ đọc lớp cửa sổ, tên tiến trình và thông tin cửa sổ. Nó đòi hỏi `UnrealWindow` và executable `Client-Win64-Shipping.exe`. Khi đã chọn EXE, đường dẫn tiến trình phải khớp đường dẫn đã lưu; khi chưa chọn, nó dùng thêm tên cửa sổ/thư mục phù hợp Wuthering Waves. Nhiều cửa sổ khớp hoặc không xác minh được executable sẽ không tự chọn. Không yêu cầu quyền quản trị để vượt lỗi nhận diện. Nếu di chuyển hoặc cập nhật game sang vị trí khác, dùng Chọn EXE để chọn lại. Nếu sau hai phút chưa thấy cửa sổ, tool hiện thông báo để bạn kiểm tra game; không tự mở lại liên tục.

Bản này không đọc/ghi bộ nhớ game, không tiêm DLL, không gửi phím/click vào game, không gọi mạng và không tạo log/ảnh màn hình. Không suy ra rằng các tính năng gameplay sẽ hoạt động từ việc thấy cửa sổ.

## Kiểm chứng

Đã build Windows x64 GUI bằng Zig 0.14.1; kiểm tra import PE và các test logic chạy trên Linux (ASan/UBSan), gồm căn giữa trên nhiều màn hình, lọc tên executable, phím không hợp lệ và khôi phục phím cũ khi trùng/lưu thất bại. Trước khi phát hành, CI Windows chạy thêm self-test của EXE, control/phím thật, lỗi trùng phím, khôi phục khi lưu thất bại, vị trí menu và vòng đời mở/ẩn/thoát của EXE. Trạng thái đạt được ghi trong `build.json`. Chưa kiểm thử với Wuthering Waves đang chạy; cần chủ tool xác nhận trong game.

`Check-Windows.cmd` chạy lại self-test Windows riêng nếu cần. Self-test chỉ tạo control ẩn và một INI tạm rồi xóa, không thao tác game. `build.json` ghi rõ giới hạn kiểm chứng.

## Build từ source

- Test logic: `python scripts/test.py` (GCC + ASan/UBSan trên Linux).
- Build: `python scripts/build.py --zig /đường/dẫn/zig`.
- Kiểm tra runtime trên Windows: `dist\WuWa-PC.exe --self-test` (exit code 0 là đạt).
- Đóng gói: `python scripts/package.py`.

Dự án độc lập. Những thay đổi SoraVynx đã dừng không nằm trong source này.
