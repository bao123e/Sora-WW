/* Native Windows UI test: synthetic windows only; no keyboard/mouse input. */
#include <assert.h>
#define wWinMain foundation_entry
#include "../src/main.c"
#undef wWinMain
struct ReservedKey {HANDLE ready,release;struct WKey key;BOOL reserved;};
static DWORD WINAPI reserve_elsewhere(void *arg){
 struct ReservedKey *r=arg;r->reserved=RegisterHotKey(NULL,900,r->key.mods|MOD_NOREPEAT,r->key.vk);SetEvent(r->ready);
 if(r->reserved){WaitForSingleObject(r->release,10000);UnregisterHotKey(NULL,900);}return 0;
}
int main(void){
 ui_language=1;wchar_t function_name[128];key_name((struct WKey){VK_F24,MOD_CONTROL|MOD_SHIFT},function_name,128);assert(!wcscmp(function_name,L"Ctrl+Shift+F24"));instance=GetModuleHandleW(NULL);assert(self_test()==0);
 INITCOMMONCONTROLSEX common={sizeof(common),ICC_HOTKEY_CLASS};assert(InitCommonControlsEx(&common));
 background=CreateSolidBrush(RGB(18,25,37));font=CreateFontW(-16,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,L"Segoe UI");
 WNDCLASSW panel={0};panel.hInstance=instance;panel.lpszClassName=L"WuWaNativeTestPanel";panel.lpfnWndProc=window_proc;assert(RegisterClassW(&panel));
 app=CreateWindowExW(0,panel.lpszClassName,L"Synthetic panel",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,0,0,700,440,NULL,NULL,instance,NULL);assert(app);assert(!IsWindowVisible(app));
 status_label=control(L"STATIC",L"",0,0,0,620,40,0);note_label=control(L"STATIC",L"",0,0,50,620,40,0);key_label=control(L"STATIC",L"",0,0,100,620,40,0);
 key_field=control(HOTKEY_CLASSW,L"",WS_TABSTOP,0,150,200,30,0);assert(key_field);SendMessageW(key_field,HKM_SETRULES,0,0);
 wchar_t temp[MAX_PATH];assert(GetTempPathW(MAX_PATH,temp));assert(GetTempFileNameW(temp,L"wwk",0,config_path));
 menu_key=(struct WKey){VK_F23,MOD_CONTROL|MOD_SHIFT};assert(write_binding(config_path,menu_key));key_registered=set_key_registration(menu_key);assert(key_registered);
 struct ReservedKey held={CreateEventW(NULL,TRUE,FALSE,NULL),CreateEventW(NULL,TRUE,FALSE,NULL),{VK_F24,MOD_CONTROL|MOD_ALT|MOD_SHIFT},FALSE};assert(held.ready&&held.release);
 HANDLE thread=CreateThread(NULL,0,reserve_elsewhere,&held,0,NULL);assert(thread);assert(WaitForSingleObject(held.ready,5000)==WAIT_OBJECT_0&&held.reserved);
 SendMessageW(key_field,HKM_SETHOTKEY,key_word(held.key),0);save_key();
 struct WKey loaded={0,0};assert(menu_key.vk==VK_F23&&key_registered);assert(read_binding(config_path,&loaded)&&loaded.vk==VK_F23);
 wchar_t note[256];GetWindowTextW(note_label,note,256);assert(wcsstr(note,L"in use"));
 SetEvent(held.release);assert(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0);CloseHandle(thread);CloseHandle(held.ready);CloseHandle(held.release);
 wchar_t saved_path[32768];wcscpy(saved_path,config_path);config_path[0]=0;save_key();assert(menu_key.vk==VK_F23&&key_registered);
 assert(RegisterHotKey(app,700,held.key.mods|MOD_NOREPEAT,held.key.vk));UnregisterHotKey(app,700);wcscpy(config_path,saved_path);
 save_key();assert(menu_key.vk==VK_F24&&menu_key.mods==held.key.mods&&key_registered);assert(read_binding(config_path,&loaded)&&loaded.vk==VK_F24&&loaded.mods==held.key.mods);
 SendMessageW(app,WM_COMMAND,DEFAULT_KEY,0);assert(LOBYTE(SendMessageW(key_field,HKM_GETHOTKEY,0,0))==VK_F1);assert(menu_key.vk==VK_F24);assert(read_binding(config_path,&loaded)&&loaded.vk==VK_F24);
 SendMessageW(key_field,HKM_SETHOTKEY,0,0);save_key();assert(menu_key.vk==VK_F24&&key_registered);
 WNDCLASSW fake={0};fake.hInstance=instance;fake.lpszClassName=L"UnrealWindow";fake.lpfnWndProc=DefWindowProcW;assert(RegisterClassW(&fake));
 HWND simulated=CreateWindowExW(0,fake.lpszClassName,L"Wuthering Waves",WS_POPUP|WS_VISIBLE,300,200,1600,900,NULL,NULL,instance,NULL);assert(simulated);
 poll_game();assert(matches==0&&unverified>=1&&!game); /* This executable is a test, not Client-Win64-Shipping.exe. */
 game=simulated;center_panel();RECT r;assert(GetWindowRect(app,&r));assert(r.left==750&&r.top==430);
 SetWindowPos(simulated,NULL,420,240,1280,720,SWP_NOZORDER|SWP_NOACTIVATE);center_panel();assert(GetWindowRect(app,&r));assert(r.left==710&&r.top==380);
 LPARAM header=MAKELPARAM(r.left+200,r.top+10);assert(window_proc(app,WM_NCHITTEST,0,header)==HTCLIENT);
 ShowWindow(app,SW_SHOWNOACTIVATE);assert(IsWindowVisible(app));SendMessageW(app,WM_CLOSE,0,0);assert(IsWindow(app)&&!IsWindowVisible(app));
 game=NULL;DestroyWindow(simulated);UnregisterClassW(fake.lpszClassName,instance);suspend_key();DeleteFileW(config_path);DestroyWindow(app);UnregisterClassW(panel.lpszClassName,instance);DeleteObject(font);DeleteObject(background);
 puts("PASS actual Windows controls, conflict/save rollback, persistence, centering, executable identity rejection and hide-on-close; no game/input execution");return 0;
}
