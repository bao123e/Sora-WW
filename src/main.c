/* SPDX-License-Identifier: MIT */
#ifndef UNICODE
#define UNICODE
#endif
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include "core.h"

#define APP_CLASS L"WuWaPCControlPanel"
#define TRAY_MESSAGE (WM_APP+1)
#define SHOW_MESSAGE (WM_APP+2)
#define MENU_HOTKEY 1
#define SAVE_KEY 101
#define DEFAULT_KEY 102
#define EXIT_APP 103
#define OPEN_APP 104
static HWND app,game,key_field,status_label,key_label,note_label;
static HINSTANCE instance;
static HFONT font,title_font;
static HBRUSH background;
static WNDPROC old_key_proc;
static HANDLE instance_mutex;
static struct WKey menu_key={VK_F1,0};
static BOOL key_registered,key_editing,tray_added,tracking=TRUE;
static int matches,unverified;
static DWORD game_pid;
static wchar_t config_path[32768];
static NOTIFYICONDATAW tray;

static BOOL set_key_registration(struct WKey key){return RegisterHotKey(app,MENU_HOTKEY,key.mods|MOD_NOREPEAT,key.vk);}
static void suspend_key(void){if(key_registered)UnregisterHotKey(app,MENU_HOTKEY);key_registered=FALSE;}
static WORD key_word(struct WKey key){return MAKEWORD(key.vk,((key.mods&MOD_CONTROL)?HOTKEYF_CONTROL:0)|((key.mods&MOD_ALT)?HOTKEYF_ALT:0)|((key.mods&MOD_SHIFT)?HOTKEYF_SHIFT:0));}
static struct WKey word_key(WORD word){struct WKey key={LOBYTE(word),0};BYTE mods=HIBYTE(word);if(mods&HOTKEYF_CONTROL)key.mods|=MOD_CONTROL;if(mods&HOTKEYF_ALT)key.mods|=MOD_ALT;if(mods&HOTKEYF_SHIFT)key.mods|=MOD_SHIFT;return key;}
static void key_name(struct WKey key,wchar_t *text,size_t cap){
 wchar_t name[80]={0};LONG scan=(LONG)MapVirtualKeyW(key.vk,MAPVK_VK_TO_VSC)<<16;
 if(key.vk==VK_INSERT||key.vk==VK_DELETE||key.vk==VK_HOME||key.vk==VK_END||key.vk==VK_PRIOR||key.vk==VK_NEXT||key.vk==VK_LEFT||key.vk==VK_RIGHT||key.vk==VK_UP||key.vk==VK_DOWN)scan|=1L<<24;
 if(!GetKeyNameTextW(scan,name,80))swprintf(name,80,L"VK %u",key.vk);
 swprintf(text,cap,L"%ls%ls%ls%ls",key.mods&MOD_CONTROL?L"Ctrl+":L"",key.mods&MOD_ALT?L"Alt+":L"",key.mods&MOD_SHIFT?L"Shift+":L"",name);
}
static void update_key_text(void){
 wchar_t binding[128],text[256];key_name(menu_key,binding,128);
 swprintf(text,256,L"Menu shortcut: %ls",binding);SetWindowTextW(key_label,text);
 swprintf(tray.szTip,128,L"WuWa PC - %ls menu",binding);if(tray_added)Shell_NotifyIconW(NIM_MODIFY,&tray);
}
static BOOL init_config(void){
 DWORD n=GetEnvironmentVariableW(L"LOCALAPPDATA",config_path,32768);if(!n||n>=32700)return FALSE;
 if(config_path[n-1]!=L'\\')wcscat(config_path,L"\\");wcscat(config_path,L"WuWaPC");
 if(!CreateDirectoryW(config_path,NULL)&&GetLastError()!=ERROR_ALREADY_EXISTS)return FALSE;
 wcscat(config_path,L"\\settings.ini");
 return TRUE;
}
/* One profile value holds the pair: save/restore is not split across two keys. */
static BOOL read_binding(const wchar_t *path,struct WKey *key){
 wchar_t raw[48];GetPrivateProfileStringW(L"Keybinds",L"Menu",L"",raw,48,path);if(!raw[0])return FALSE;
 wchar_t *end;unsigned long vk=wcstoul(raw,&end,10);if(end==raw||*end!=L':')return FALSE;
 wchar_t *start=end+1;unsigned long mods=wcstoul(start,&end,10);if(end==start||*end||vk>254||mods>7)return FALSE;
 struct WKey parsed={(unsigned)vk,(unsigned)mods};if(!key_valid(parsed))return FALSE;*key=parsed;return TRUE;
}
static BOOL write_binding(const wchar_t *path,struct WKey key){wchar_t text[48];swprintf(text,48,L"%u:%u",key.vk,key.mods);return WritePrivateProfileStringW(L"Keybinds",L"Menu",text,path);}
static int reserve_menu_key(void *ctx,struct WKey key){(void)ctx;return set_key_registration(key);}
static void release_menu_key(void *ctx){(void)ctx;UnregisterHotKey(app,MENU_HOTKEY);}
static int persist_menu_key(void *ctx,struct WKey key){(void)ctx;return config_path[0]&&write_binding(config_path,key);}
static void save_key(void){
 struct WKey next=word_key((WORD)SendMessageW(key_field,HKM_GETHOTKEY,0,0));
 struct WKeyState state={menu_key,key_registered};struct WKeyOps ops={NULL,reserve_menu_key,release_menu_key,persist_menu_key};
 enum WKeyResult result=key_apply(&state,next,ops);menu_key=state.key;key_registered=state.registered;
 if(result==KEY_INVALID)SetWindowTextW(note_label,L"Choose a key. Use Ctrl, Alt or Shift if needed.");
 else if(result==KEY_BUSY)SetWindowTextW(note_label,L"That shortcut is in use. The previous shortcut is kept.");
 else if(result==KEY_SAVE_FAILED)SetWindowTextW(note_label,L"Could not save settings. The previous shortcut is kept.");
 else {update_key_text();SetWindowTextW(note_label,L"Saved. This shortcut will be used next time too.");}
}
static BOOL contains_wuwa(const wchar_t *title){wchar_t lowered[256];wcsncpy(lowered,title,255);lowered[255]=0;CharLowerBuffW(lowered,(DWORD)wcslen(lowered));return wcsstr(lowered,L"wuthering")&&wcsstr(lowered,L"waves");}
static BOOL CALLBACK find_game(HWND hwnd,LPARAM unused){
 (void)unused;if(!IsWindowVisible(hwnd)||GetWindow(hwnd,GW_OWNER))return TRUE;
 wchar_t klass[64],title[256];if(!GetClassNameW(hwnd,klass,64)||wcscmp(klass,L"UnrealWindow"))return TRUE;
 GetWindowTextW(hwnd,title,256);
 DWORD pid=0;GetWindowThreadProcessId(hwnd,&pid);HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
 if(!process){unverified++;return TRUE;}
 wchar_t path[32768];DWORD length=32768;BOOL ok=QueryFullProcessImageNameW(process,0,path,&length);CloseHandle(process);
 if(!ok){unverified++;return TRUE;}
 wchar_t *name=wcsrchr(path,L'\\');name=name?name+1:path;
 char narrow[128];int bytes=WideCharToMultiByte(CP_UTF8,0,name,-1,narrow,128,NULL,NULL);
 if(!bytes||!known_game_image(narrow)||(!contains_wuwa(title)&&!contains_wuwa(path))){unverified++;return TRUE;}
 matches++;game=hwnd;game_pid=pid;return TRUE;
}
static void poll_game(void){
 if(!tracking)return;matches=unverified=0;game=NULL;game_pid=0;EnumWindows(find_game,0);
 wchar_t text[256];
 if(matches==1)swprintf(text,256,L"Game window found (read only) - PID %lu",(unsigned long)game_pid);
 else if(matches>1){game=NULL;wcscpy(text,L"Multiple game windows found. Close the extra instance first.");}
 else if(unverified)wcscpy(text,L"Unreal window found; executable identity not verified.");
 else wcscpy(text,L"Waiting for the Wuthering Waves game window...");
 SetWindowTextW(status_label,text);
}
static void position_for(HWND hwnd,int width,int height,int *x,int *y){
 RECT area;HWND target=game;
 if(target&&IsWindow(target)&&!IsIconic(target)&&GetClientRect(target,&area)){
  POINT origin={0,0};if(ClientToScreen(target,&origin))OffsetRect(&area,origin.x,origin.y);else target=NULL;
 }else target=NULL;
 if(!target){MONITORINFO info={0};info.cbSize=sizeof(info);if(GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&info))area=info.rcWork;else SystemParametersInfoW(SPI_GETWORKAREA,0,&area,0);}
 struct WPoint center=centered((struct WRect){area.left,area.top,area.right,area.bottom},width,height);*x=(int)center.x;*y=(int)center.y;
}
static void center_panel(void){RECT rect;GetWindowRect(app,&rect);int x,y;position_for(app,rect.right-rect.left,rect.bottom-rect.top,&x,&y);if(x!=rect.left||y!=rect.top)SetWindowPos(app,NULL,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}
static void show_panel(void){poll_game();center_panel();ShowWindow(app,SW_SHOWNORMAL);SetForegroundWindow(app);}
static LRESULT CALLBACK key_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_SETFOCUS){key_editing=TRUE;suspend_key();}
 if(msg==WM_KILLFOCUS){key_editing=FALSE;if(!key_registered)key_registered=set_key_registration(menu_key);}
 return CallWindowProcW(old_key_proc,hwnd,msg,wp,lp);
}
static HWND control(const wchar_t *klass,const wchar_t *text,DWORD style,int x,int y,int w,int h,int id){
 HWND c=CreateWindowExW(!wcscmp(klass,HOTKEY_CLASSW)?WS_EX_CLIENTEDGE:0,klass,text,WS_CHILD|WS_VISIBLE|style,x,y,w,h,app,(HMENU)(INT_PTR)id,instance,NULL);SendMessageW(c,WM_SETFONT,(WPARAM)font,TRUE);return c;
}
static BOOL fallback_menu_down(void){
 if(!(GetAsyncKeyState((int)menu_key.vk)&0x8000))return FALSE;unsigned mods=0;
 if(GetAsyncKeyState(VK_CONTROL)&0x8000)mods|=MOD_CONTROL;if(GetAsyncKeyState(VK_MENU)&0x8000)mods|=MOD_ALT;if(GetAsyncKeyState(VK_SHIFT)&0x8000)mods|=MOD_SHIFT;return mods==menu_key.mods;
}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 switch(msg){
 case SHOW_MESSAGE:show_panel();return 0;
 case WM_HOTKEY:if(wp==MENU_HOTKEY&&!key_editing){if(IsWindowVisible(hwnd))ShowWindow(hwnd,SW_HIDE);else show_panel();}return 0;
 case WM_COMMAND:
  if(LOWORD(wp)==SAVE_KEY)save_key();
  else if(LOWORD(wp)==DEFAULT_KEY){SendMessageW(key_field,HKM_SETHOTKEY,key_word((struct WKey){VK_F1,0}),0);SetWindowTextW(note_label,L"Default selected. Click Save to apply it.");}
  else if(LOWORD(wp)==EXIT_APP)DestroyWindow(hwnd);
  else if(LOWORD(wp)==OPEN_APP)show_panel();return 0;
 case TRAY_MESSAGE:
  if(lp==WM_LBUTTONUP)show_panel();
  if(lp==WM_RBUTTONUP){HMENU popup=CreatePopupMenu();AppendMenuW(popup,MF_STRING,OPEN_APP,L"Open menu");AppendMenuW(popup,MF_STRING,EXIT_APP,L"Exit");POINT point;GetCursorPos(&point);SetForegroundWindow(hwnd);TrackPopupMenu(popup,TPM_RIGHTBUTTON,point.x,point.y,0,hwnd,NULL);DestroyMenu(popup);PostMessageW(hwnd,WM_NULL,0,0);}return 0;
 case WM_TIMER:
  if(wp==1){poll_game();if(IsWindowVisible(hwnd))center_panel();}
  if(wp==2&&!key_registered&&!key_editing){static BOOL previous;BOOL down=fallback_menu_down();if(down&&!previous){if(IsWindowVisible(hwnd))ShowWindow(hwnd,SW_HIDE);else show_panel();}previous=down;}return 0;
 case WM_NCHITTEST:{LRESULT hit=DefWindowProcW(hwnd,msg,wp,lp);return hit==HTCAPTION?HTCLIENT:hit;}
 case WM_SYSCOMMAND:if((wp&0xfff0)==SC_MOVE||(wp&0xfff0)==SC_SIZE)return 0;break;
 case WM_WINDOWPOSCHANGING:{WINDOWPOS *pos=(WINDOWPOS*)lp;if(!(pos->flags&SWP_NOMOVE)){RECT r;GetWindowRect(hwnd,&r);position_for(hwnd,(pos->flags&SWP_NOSIZE)?r.right-r.left:pos->cx,(pos->flags&SWP_NOSIZE)?r.bottom-r.top:pos->cy,&pos->x,&pos->y);}return 0;}
 case WM_CTLCOLORSTATIC:SetTextColor((HDC)wp,RGB(222,232,242));SetBkColor((HDC)wp,RGB(18,25,37));return (LRESULT)background;
 case WM_CLOSE:ShowWindow(hwnd,SW_HIDE);return 0;
 case WM_DESTROY:
  tracking=FALSE;KillTimer(hwnd,1);KillTimer(hwnd,2);suspend_key();if(tray_added)Shell_NotifyIconW(NIM_DELETE,&tray);PostQuitMessage(0);return 0;
 }
 return DefWindowProcW(hwnd,msg,wp,lp);
}
static int self_test(void){
 struct WKey key={VK_F1,0};if(!key_valid(key))return 1;
 INITCOMMONCONTROLSEX common={sizeof(common),ICC_HOTKEY_CLASS};if(!InitCommonControlsEx(&common))return 2;
 HWND host=CreateWindowExW(0,L"STATIC",L"Self-test",0,0,0,200,100,NULL,NULL,instance,NULL);if(!host)return 3;
 HWND field=CreateWindowExW(0,HOTKEY_CLASSW,L"",WS_CHILD,0,0,180,30,host,NULL,instance,NULL);if(!field){DestroyWindow(host);return 4;}
 struct WKey test={VK_F24,MOD_CONTROL|MOD_SHIFT};SendMessageW(field,HKM_SETHOTKEY,key_word(test),0);struct WKey actual=word_key((WORD)SendMessageW(field,HKM_GETHOTKEY,0,0));
 int result=(actual.vk!=test.vk||actual.mods!=test.mods)?5:0;
 wchar_t temp[MAX_PATH],path[MAX_PATH];if(!GetTempPathW(MAX_PATH,temp)||!GetTempFileNameW(temp,L"wwt",0,path))result=6;
 else {struct WKey loaded={0,0};if(!write_binding(path,test)||!read_binding(path,&loaded)||loaded.vk!=test.vk||loaded.mods!=test.mods)result=7;DeleteFileW(path);}
 DestroyWindow(host);return result;
}
int WINAPI wWinMain(HINSTANCE module,HINSTANCE previous,PWSTR command,int show){
 (void)previous;(void)show;instance=module;
 if(!wcscmp(command,L"--self-test"))return self_test();
 instance_mutex=CreateMutexW(NULL,FALSE,L"Local\\WuWaPC.ControlPanel");if(!instance_mutex)return 1;
 if(GetLastError()==ERROR_ALREADY_EXISTS){HWND existing=FindWindowW(APP_CLASS,NULL);if(existing)PostMessageW(existing,SHOW_MESSAGE,0,0);CloseHandle(instance_mutex);return 0;}
 typedef BOOL (WINAPI *SetDpiContext)(HANDLE);HMODULE user=GetModuleHandleW(L"user32.dll");FARPROC raw=GetProcAddress(user,"SetProcessDpiAwarenessContext");SetDpiContext dpi=NULL;memcpy(&dpi,&raw,sizeof(dpi));if(dpi)dpi((HANDLE)(INT_PTR)-4);
 INITCOMMONCONTROLSEX common={sizeof(common),ICC_HOTKEY_CLASS};if(!InitCommonControlsEx(&common)){CloseHandle(instance_mutex);return 2;}
 background=CreateSolidBrush(RGB(18,25,37));font=CreateFontW(-16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");title_font=CreateFontW(-28,0,0,0,FW_SEMIBOLD,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,0,L"Segoe UI");
 WNDCLASSW klass={0};klass.hInstance=module;klass.lpfnWndProc=window_proc;klass.lpszClassName=APP_CLASS;klass.hCursor=LoadCursorW(NULL,IDC_ARROW);klass.hIcon=LoadIconW(NULL,IDI_APPLICATION);klass.hbrBackground=background;
 if(!RegisterClassW(&klass)){CloseHandle(instance_mutex);return 3;}
 app=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_TOPMOST,APP_CLASS,L"WuWa PC - Foundation 0.1",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,0,0,700,440,NULL,NULL,module,NULL);if(!app){CloseHandle(instance_mutex);return 4;}
 init_config();struct WKey saved;if(config_path[0]&&read_binding(config_path,&saved))menu_key=saved;
 HWND heading=control(L"STATIC",L"WuWa PC",0,28,22,400,44,0);SendMessageW(heading,WM_SETFONT,(WPARAM)title_font,TRUE);
 control(L"STATIC",L"STEP 1  /  MENU AND WINDOW CONNECTION",0,30,74,620,24,0);
 status_label=control(L"STATIC",L"Waiting for the game window...",0,30,118,620,44,0);
 control(L"STATIC",L"This build checks the menu only. Gameplay features are not available yet.",0,30,170,620,44,0);
 key_label=control(L"STATIC",L"",0,30,230,620,24,0);
 key_field=control(HOTKEY_CLASSW,L"",WS_TABSTOP,30,266,290,30,0);SendMessageW(key_field,HKM_SETRULES,0,0);SendMessageW(key_field,HKM_SETHOTKEY,key_word(menu_key),0);old_key_proc=(WNDPROC)SetWindowLongPtrW(key_field,GWLP_WNDPROC,(LONG_PTR)key_proc);
 control(L"BUTTON",L"Save",WS_TABSTOP,336,264,90,34,SAVE_KEY);control(L"BUTTON",L"Default",WS_TABSTOP,438,264,94,34,DEFAULT_KEY);control(L"BUTTON",L"Exit",WS_TABSTOP,546,264,100,34,EXIT_APP);
 note_label=control(L"STATIC",L"Click the shortcut field, press your key combination, then Save.",0,30,320,620,54,0);
 tray.cbSize=sizeof(tray);tray.hWnd=app;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=TRAY_MESSAGE;tray.hIcon=klass.hIcon;
 update_key_text();tray_added=Shell_NotifyIconW(NIM_ADD,&tray);key_registered=set_key_registration(menu_key);poll_game();center_panel();SetTimer(app,1,750,NULL);SetTimer(app,2,50,NULL);
 if(!key_registered){show_panel();SetWindowTextW(note_label,L"Menu shortcut is in use. Choose a different shortcut and Save.");}
 else if(!tray_added)SetWindowTextW(note_label,L"Tray icon unavailable. Your menu shortcut still works.");
 MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){if(!IsDialogMessageW(app,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
 DeleteObject(font);DeleteObject(title_font);DeleteObject(background);CloseHandle(instance_mutex);return 0;
}
