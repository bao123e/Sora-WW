# WuWa PC — menu và mở game

Bản 0.4.0-experimental thêm chuẩn bị gói C# thử nghiệm cho No Cooldown và chặn hai luồng sát thương cục bộ. Hiệu lực trong WuWa thật chưa xác nhận; không có thông báo “gameplay READY”. Damage Multiplier chưa hỗ trợ.

Thông tin chủ tool đã cung cấp ngày 26/11/2026:

- Launcher chính thức: 3.1.0.1.
- Màn hình game: `OS_Windows_Product_3.8.0_9244589_3.8.13_9534983 * SD`.
- Khu vực SEA.
- EXE đã gửi có tài nguyên phiên bản UE4 4.26; chưa xác minh API đồ họa khi game chạy. Chuỗi trên là bằng chứng từ ảnh màn hình, không phải chứng nhận tương thích executable.

## Dùng thử

1. Thoát hẳn bản tool cũ bằng **Thoát**, giải nén toàn bộ ZIP vào thư mục riêng. Giữ cả `WuWa-PC.exe` và `Sora-Gameplay.exe` trong cùng thư mục; không cần cài Python.
2. Chạy `Start-WuWa-PC.cmd`, mở menu bằng phím đã lưu (mặc định F1). Trong Tổng quan, chọn EXE tại `Client\Binaries\Win64\Client-Win64-Shipping.exe`.
3. **Đóng game trước khi áp dụng hoặc hoàn tác gói thử.** Bật No Cooldown hoặc Chặn sát thương rồi bấm **Mở game**. Backend xác minh đúng pakchunk18 đã phân tích và tạo riêng `Client\Content\Paks\zz_SoraWW_Experimental_P.pak` chứa DLL được chuẩn bị trên máy bạn. Các PAK/SIG/EXE gốc giữ nguyên. Tool không thay kiểm tra chữ ký hoặc tìm cách ép client chấp nhận gói.
4. Lựa chọn áp dụng ở lần mở game tiếp theo; không phải công tắc tức thời trong game. Menu vẫn đáp ứng trong lúc chuẩn bị. Khi chuẩn bị xong, tool mở EXE với `-krqlv=sd` và ẩn menu sau khi nhận diện cửa sổ.
5. **Thử No Cooldown riêng trước** để kiểm tra client có nạp gói không. Chuẩn bị gói thành công và tìm thấy cửa sổ không chứng minh gameplay đã có hiệu lực. Nếu game không mở hoặc không thay đổi, đóng game rồi dùng **Hoàn tác gói thử**. Hoàn tác chỉ xóa gói có hash khớp bản do tool ghi, không tự xóa gói đã bị thay đổi hoặc không có hồ sơ sở hữu.
6. Chặn sát thương chỉ thử hai hàm cục bộ `ProcessDamage` và `FallInjure` của role thuộc người chơi; chưa xác nhận bảo vệ HP từ server, mọi loại sát thương hay đồng bộ mạng. No Cooldown cũng cần xác nhận hoạt động, số lượt kỹ năng và giao diện trong game thật.
7. Phím menu vẫn tùy chỉnh ở tab Phím tắt → bấm ô phím → nhấn tổ hợp → **Lưu**. Nút × chỉ ẩn menu; **Thoát** đóng tool. Thoát trong lúc chuẩn bị sẽ dừng tiến trình chuẩn bị riêng của tool, không đóng game.

Đường dẫn, phím và lựa chọn được lưu tại `%LOCALAPPDATA%\WuWaPC`. Hồ sơ gói thử ở `gameplay-trial.json` cho phép hoàn tác. Nếu game cập nhật làm đổi hash pakchunk18, tool từ chối áp dụng; không dùng offset phỏng đoán cho bản khác. Không đăng PAK/DLL đầu ra lên GitHub vì chứa code game.

## Giao diện theo ảnh tham khảo

Nền tối có độ trong nhẹ, thanh tiêu đề xanh, tab Tổng quan / Phím tắt, nhóm có đường phân cách và thanh đổi phím/ngôn ngữ luôn ở cuối. Tab Tổng quan có nút chọn độ đậm menu (85/90/94/100%). Ghim giữ menu ở trên các cửa sổ khác. Chuyển tiếng Việt / English ở thanh cuối. Những tùy chọn này được lưu cùng cấu hình phím; phím đã chọn ở bản 0.1 vẫn dùng được.

Đổi phím ở tab Phím tắt hoặc nút Đổi phím phía dưới. Cuộn con lăn hoặc kéo thanh cuộn khi nội dung dài. Ảnh tham khảo chỉ được dùng cho giao diện; không có tùy chọn Frame Generation, DLSS hay GPU giả lập.

## Cần kiểm tra ở lượt đầu

- Menu có bắt đầu ẩn và mở bằng phím đã chọn không?
- Menu có nằm giữa cửa sổ game không?
- Đổi phím, thoát hẳn, mở lại: phím có được giữ không?
- Dòng trạng thái có hiện **Game window found** không? Nếu chưa, gửi đúng dòng trạng thái; chưa cần gửi đường dẫn hoặc file game.

Bộ dò chỉ đọc lớp cửa sổ, tên tiến trình và thông tin cửa sổ. Nó đòi hỏi `UnrealWindow` và executable `Client-Win64-Shipping.exe`. Khi đã chọn EXE, đường dẫn tiến trình phải khớp đường dẫn đã lưu; khi chưa chọn, nó dùng thêm tên cửa sổ/thư mục phù hợp Wuthering Waves. Nhiều cửa sổ khớp hoặc không xác minh được executable sẽ không tự chọn. Không yêu cầu quyền quản trị để vượt lỗi nhận diện. Nếu di chuyển hoặc cập nhật game sang vị trí khác, dùng Chọn EXE để chọn lại. Nếu sau hai phút chưa thấy cửa sổ, tool hiện thông báo để bạn kiểm tra game; không tự mở lại liên tục.

Bản này không đọc/ghi bộ nhớ game, không tiêm DLL, không gửi phím/click vào game, không gọi mạng và không chụp màn hình. Nó tạo riêng gói thử khi bạn bật lựa chọn và bấm Mở game; lưu hồ sơ hash và kết quả chuẩn bị cục bộ. Không suy ra rằng các tính năng gameplay sẽ hoạt động từ việc thấy cửa sổ.

## Kiểm chứng

Đã build Windows x64 GUI bằng Zig 0.14.1; kiểm tra import PE và các test logic chạy trên Linux (ASan/UBSan), gồm căn giữa trên nhiều màn hình, lọc tên executable, phím không hợp lệ và khôi phục phím cũ khi trùng/lưu thất bại. Trước khi phát hành, CI Windows chạy thêm self-test của EXE, control/phím thật, lỗi trùng phím, khôi phục khi lưu thất bại, vị trí menu và vòng đời mở/ẩn/thoát của EXE. Trạng thái đạt được ghi trong `build.json`. Chưa kiểm thử với Wuthering Waves đang chạy; cần chủ tool xác nhận trong game.

`Check-Windows.cmd` chạy lại self-test Windows riêng nếu cần. Self-test chỉ tạo control ẩn và một INI tạm rồi xóa, không thao tác game. `build.json` ghi rõ giới hạn kiểm chứng.

## Build từ source

- Test logic: `python scripts/test.py` (GCC + ASan/UBSan trên Linux).
- Build: `python scripts/build.py --zig /đường/dẫn/zig`.
- Kiểm tra runtime trên Windows: `dist\WuWa-PC.exe --self-test` (exit code 0 là đạt).
- Đóng gói: `python scripts/package.py`.

Dự án độc lập. Những thay đổi SoraVynx đã dừng không nằm trong source này.
