# WuWa PC — menu và mở game

Bản 0.4.1-recovery khôi phục trạng thái trước gói thử 0.4 sau báo cáo game đóng. Nó tạm dừng hai lựa chọn cũ, gỡ riêng gói do tool tạo có hash còn khớp trước khi mở game. Chưa xác định nguyên nhân gốc của lần văng và chưa có backend bật/tắt gameplay trong phiên game.

Thông tin chủ tool đã cung cấp ngày 26/11/2026:

- Launcher chính thức: 3.1.0.1.
- Màn hình game: `OS_Windows_Product_3.8.0_9244589_3.8.13_9534983 * SD`.
- Khu vực SEA.
- EXE đã gửi có tài nguyên phiên bản UE4 4.26; chưa xác minh API đồ họa khi game chạy. Chuỗi trên là bằng chứng từ ảnh màn hình, không phải chứng nhận tương thích executable.

## Khôi phục

1. Đóng game và **Thoát** hẳn tool 0.4. Giải nén toàn bộ ZIP mới vào thư mục riêng, giữ `WuWa-PC.exe` và `Sora-Gameplay.exe` cạnh nhau.
2. Chạy `Start-WuWa-PC.cmd`, mở menu bằng phím đã lưu. Đường dẫn EXE và phím tùy chỉnh được giữ nguyên. Hai lựa chọn cũ hiển thị **Tạm dừng**.
3. Bấm **Khôi phục / Mở game**. Nếu có gói thử do tool ghi, backend đối chiếu hồ sơ/hash rồi gỡ `Client\Content\Paks\zz_SoraWW_Experimental_P.pak` trước khi mở game. PAK/SIG/EXE gốc không bị sửa.
4. Nếu game vẫn đóng sau khi đã gỡ gói, cần kiểm tra lỗi/crash log thực tế. Bản này không khẳng định đã sửa nguyên nhân gốc. Không bật lại bản vá 0.4 để thử lặp.
5. Nếu gói không có hồ sơ sở hữu hoặc hash đã thay đổi, tool sẽ báo lỗi và không tự xóa. Không xóa các PAK gốc để xử lý.

Đây là bản khôi phục, không có chức năng gameplay đang hoạt động. Việc bật/tắt ngay trong phiên game cần một backend khác được kiểm chứng; các lựa chọn 0.4 chỉ sửa file trước lúc game khởi động và không đáp ứng yêu cầu đó.

## Giao diện theo ảnh tham khảo

Nền tối có độ trong nhẹ, thanh tiêu đề xanh, tab Tổng quan / Phím tắt, nhóm có đường phân cách và thanh đổi phím/ngôn ngữ luôn ở cuối. Tab Tổng quan có nút chọn độ đậm menu (85/90/94/100%). Ghim giữ menu ở trên các cửa sổ khác. Chuyển tiếng Việt / English ở thanh cuối. Những tùy chọn này được lưu cùng cấu hình phím; phím đã chọn ở bản 0.1 vẫn dùng được.

Đổi phím ở tab Phím tắt hoặc nút Đổi phím phía dưới. Cuộn con lăn hoặc kéo thanh cuộn khi nội dung dài. Ảnh tham khảo chỉ được dùng cho giao diện; không có tùy chọn Frame Generation, DLSS hay GPU giả lập.

## Cần kiểm tra ở lượt đầu

- Menu có bắt đầu ẩn và mở bằng phím đã chọn không?
- Menu có nằm giữa cửa sổ game không?
- Đổi phím, thoát hẳn, mở lại: phím có được giữ không?
- Dòng trạng thái có hiện **Game window found** không? Nếu chưa, gửi đúng dòng trạng thái; chưa cần gửi đường dẫn hoặc file game.

Bộ dò chỉ đọc lớp cửa sổ, tên tiến trình và thông tin cửa sổ. Nó đòi hỏi `UnrealWindow` và executable `Client-Win64-Shipping.exe`. Khi đã chọn EXE, đường dẫn tiến trình phải khớp đường dẫn đã lưu; khi chưa chọn, nó dùng thêm tên cửa sổ/thư mục phù hợp Wuthering Waves. Nhiều cửa sổ khớp hoặc không xác minh được executable sẽ không tự chọn. Không yêu cầu quyền quản trị để vượt lỗi nhận diện. Nếu di chuyển hoặc cập nhật game sang vị trí khác, dùng Chọn EXE để chọn lại. Nếu sau hai phút chưa thấy cửa sổ, tool hiện thông báo để bạn kiểm tra game; không tự mở lại liên tục.

Bản này không đọc/ghi bộ nhớ game, không tiêm DLL, không gửi phím/click vào game, không gọi mạng và không chụp màn hình. Bản khôi phục chỉ gỡ gói có hash khớp hồ sơ cục bộ của tool; không tạo gói mới. Không suy ra rằng các tính năng gameplay sẽ hoạt động từ việc thấy cửa sổ.

## Kiểm chứng

Đã build Windows x64 GUI bằng Zig 0.14.1; kiểm tra import PE và các test logic chạy trên Linux (ASan/UBSan), gồm căn giữa trên nhiều màn hình, lọc tên executable, phím không hợp lệ và khôi phục phím cũ khi trùng/lưu thất bại. Trước khi phát hành, CI Windows chạy thêm self-test của EXE, control/phím thật, lỗi trùng phím, khôi phục khi lưu thất bại, vị trí menu và vòng đời mở/ẩn/thoát của EXE. Trạng thái đạt được ghi trong `build.json`. Chưa kiểm thử với Wuthering Waves đang chạy; cần chủ tool xác nhận trong game.

`Check-Windows.cmd` chạy lại self-test Windows riêng nếu cần. Self-test chỉ tạo control ẩn và một INI tạm rồi xóa, không thao tác game. `build.json` ghi rõ giới hạn kiểm chứng.

## Build từ source

- Test logic: `python scripts/test.py` (GCC + ASan/UBSan trên Linux).
- Build: `python scripts/build.py --zig /đường/dẫn/zig`.
- Kiểm tra runtime trên Windows: `dist\WuWa-PC.exe --self-test` (exit code 0 là đạt).
- Đóng gói: `python scripts/package.py`.

Dự án độc lập. Những thay đổi SoraVynx đã dừng không nằm trong source này.
