/* SPDX-License-Identifier: MIT */
#ifndef WUWA_LAUNCHER_H
#define WUWA_LAUNCHER_H
/* ASCII hex preserves UTF-16 paths in existing ANSI or Unicode profile files. */
static BOOL save_game_path(const wchar_t *path){
 if(!config_path[0]||wcslen(path)>=32768)return FALSE;
 size_t length=wcslen(path);wchar_t *encoded=malloc((length*4+1)*sizeof(wchar_t));if(!encoded)return FALSE;
 const wchar_t digits[]=L"0123456789abcdef";
 for(size_t i=0;i<length;i++)for(int j=0;j<4;j++)encoded[i*4+j]=digits[((unsigned)path[i]>>(12-j*4))&15];
 encoded[length*4]=0;BOOL ok=WritePrivateProfileStringW(L"Game",L"ExecutableUtf16Hex",encoded,config_path);free(encoded);return ok;
}
static int path_hex(wchar_t c){if(c>=L'0'&&c<=L'9')return c-L'0';if(c>=L'a'&&c<=L'f')return c-L'a'+10;return -1;}
static void load_game_path(void){
 game_path[0]=0;if(!config_path[0])return;
 DWORD cap=32768*4+1;wchar_t *encoded=calloc(cap,sizeof(wchar_t));if(!encoded)return;
 DWORD count=GetPrivateProfileStringW(L"Game",L"ExecutableUtf16Hex",L"",encoded,cap,config_path);
 if(count&&count<cap-1&&count%4==0){BOOL valid=TRUE;for(DWORD i=0;i<count/4;i++){unsigned value=0;for(int j=0;j<4;j++){int digit=path_hex(encoded[i*4+j]);if(digit<0){valid=FALSE;break;}value=(value<<4)|(unsigned)digit;}if(!value)valid=FALSE;game_path[i]=(wchar_t)value;}game_path[count/4]=0;if(!valid)game_path[0]=0;}
 free(encoded);
}
static BOOL valid_game_path(const wchar_t *path){
 const wchar_t *name=wcsrchr(path,L'\\');if(!name||_wcsicmp(name+1,L"Client-Win64-Shipping.exe"))return FALSE;
 DWORD attributes=GetFileAttributesW(path),type=0;
 return attributes!=INVALID_FILE_ATTRIBUTES&&!(attributes&FILE_ATTRIBUTE_DIRECTORY)&&GetBinaryTypeW(path,&type)&&type==SCS_64BIT_BINARY;
}
static void choose_game(void){
 wchar_t selected[32768]={0};if(game_path[0])wcscpy(selected,game_path);
 OPENFILENAMEW dialog={0};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=app;dialog.lpstrFilter=L"Wuthering Waves (Client-Win64-Shipping.exe)\0Client-Win64-Shipping.exe\0\0";dialog.lpstrFile=selected;dialog.nMaxFile=32768;dialog.lpstrTitle=ui_text(L"Chọn EXE của Wuthering Waves",L"Select the Wuthering Waves EXE");dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
 if(!GetOpenFileNameW(&dialog)){if(CommDlgExtendedError())wcscpy(launch_notice,ui_text(L"Không mở được hộp chọn file.",L"Could not open the file picker."));}
 else if(!valid_game_path(selected))wcscpy(launch_notice,ui_text(L"Chọn Client-Win64-Shipping.exe bản Windows 64 bit.",L"Choose the 64-bit Windows Client-Win64-Shipping.exe."));
 else if(!save_game_path(selected))wcscpy(launch_notice,ui_text(L"Không lưu được đường dẫn. Lựa chọn cũ được giữ nguyên.",L"Could not save the path. Your previous selection is kept."));
 else {wcscpy(game_path,selected);wcscpy(launch_notice,ui_text(L"Đã lưu đường dẫn. Bấm Mở game khi sẵn sàng.",L"Path saved. Select Open game when ready."));poll_game();}
 InvalidateRect(ui_body,NULL,FALSE);
}
/* Detect the EXE even while its window is still loading. A same-name process blocks launching another instance. */
static int game_process_state(void){
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(snapshot==INVALID_HANDLE_VALUE)return -1;
 PROCESSENTRY32W entry={0};entry.dwSize=sizeof(entry);int result=0;
 if(!Process32FirstW(snapshot,&entry))result=-1;
 else do{if(!_wcsicmp(entry.szExeFile,L"Client-Win64-Shipping.exe")){result=1;break;}}while(Process32NextW(snapshot,&entry));
 CloseHandle(snapshot);return result;
}
static void launch_game(void){
 poll_game();
 if(matches==1){ShowWindow(app,SW_HIDE);if(IsIconic(game))ShowWindow(game,SW_RESTORE);SetForegroundWindow(game);return;}
 if(launch_pending){wcscpy(launch_notice,ui_text(L"Đang chờ game mở…",L"Waiting for the game to open…"));}
 else if(!valid_game_path(game_path)){wcscpy(launch_notice,ui_text(L"Chọn EXE trước; nếu đã di chuyển game, hãy chọn lại đường dẫn.",L"Choose the EXE first. If the game moved, select its new path."));}
 else {int state=game_process_state();
  if(state<0)wcscpy(launch_notice,ui_text(L"Không kiểm tra được tiến trình. Chưa mở thêm game.",L"Could not check processes. No additional game was launched."));
  else if(state>0){launch_pending=TRUE;launch_started=GetTickCount64();wcscpy(launch_notice,ui_text(L"Game đang chạy. Chờ cửa sổ game; không mở thêm phiên.",L"Game is already running. Waiting for its window."));}
  else {wchar_t folder[32768];wcscpy(folder,game_path);wchar_t *slash=wcsrchr(folder,L'\\');if(slash)*slash=0;
   SHELLEXECUTEINFOW info={0};info.cbSize=sizeof(info);info.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;info.hwnd=app;info.lpVerb=L"open";info.lpFile=game_path;info.lpParameters=L"-krqlv=sd";info.lpDirectory=folder;info.nShow=SW_SHOWNORMAL;
   if(ShellExecuteExW(&info)){if(info.hProcess)CloseHandle(info.hProcess);launch_pending=TRUE;launch_started=GetTickCount64();wcscpy(launch_notice,ui_text(L"Đã gửi yêu cầu mở. Menu sẽ ẩn khi tìm thấy cửa sổ game.",L"Launch requested. The menu will hide when the game window appears."));}
   else {DWORD error=GetLastError();swprintf(launch_notice,256,ui_text(L"Không mở được game (Windows %lu).",L"Could not open the game (Windows %lu)."),(unsigned long)error);}
  }
 }
 if(ui_ready)InvalidateRect(ui_body,NULL,FALSE);
}
#endif
