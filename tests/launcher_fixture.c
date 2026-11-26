/* Synthetic launch target for CI; no real game, network or account data. */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <wchar.h>
static LRESULT CALLBACK fixture_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
 if(msg==WM_TIMER){DestroyWindow(hwnd);return 0;}
 if(msg==WM_DESTROY){PostQuitMessage(0);return 0;}
 return DefWindowProcW(hwnd,msg,wp,lp);
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE previous,PWSTR command,int show){
 (void)previous;(void)command;(void)show;
 wchar_t exe[32768],folder[32768];if(!GetModuleFileNameW(NULL,exe,32768)||!GetCurrentDirectoryW(32768,folder))return 3;
 wchar_t *slash=wcsrchr(exe,L'\\');if(!slash)return 4;*slash=0;if(_wcsicmp(exe,folder))return 5;
 Sleep(750);
 WNDCLASSW klass={0};klass.hInstance=instance;klass.lpfnWndProc=fixture_proc;klass.lpszClassName=L"UnrealWindow";
 if(!RegisterClassW(&klass))return 1;
 HWND hwnd=CreateWindowW(klass.lpszClassName,L"Synthetic launcher fixture",WS_OVERLAPPEDWINDOW|WS_VISIBLE,40,40,800,500,NULL,NULL,instance,NULL);if(!hwnd)return 2;
 SetTimer(hwnd,1,30000,NULL);MSG msg;while(GetMessageW(&msg,NULL,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}return 0;
}
