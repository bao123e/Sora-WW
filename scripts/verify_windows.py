"""Verify native controls and actual packaged app lifecycle without a real game."""
from pathlib import Path
import argparse,ctypes,ctypes.wintypes as wt,hashlib,json,os,subprocess,tempfile,time
from capture_ui_windows import capture
ROOT=Path(__file__).resolve().parents[1]
def wait_until(predicate,message,seconds=12):
    deadline=time.monotonic()+seconds
    while time.monotonic()<deadline:
        value=predicate()
        if value:return value
        time.sleep(.1)
    raise AssertionError(message)
def lifecycle(exe):
    user=ctypes.WinDLL('user32',use_last_error=True)
    user.FindWindowW.argtypes=[wt.LPCWSTR,wt.LPCWSTR];user.FindWindowW.restype=wt.HWND
    user.GetWindowThreadProcessId.argtypes=[wt.HWND,ctypes.POINTER(wt.DWORD)]
    user.IsWindowVisible.argtypes=[wt.HWND];user.IsWindowVisible.restype=wt.BOOL
    user.PostMessageW.argtypes=[wt.HWND,wt.UINT,wt.WPARAM,wt.LPARAM];user.PostMessageW.restype=wt.BOOL
    user.GetClassNameW.argtypes=[wt.HWND,wt.LPWSTR,ctypes.c_int]
    user.GetDlgItem.argtypes=[wt.HWND,ctypes.c_int];user.GetDlgItem.restype=wt.HWND
    user.GetWindowRect.argtypes=[wt.HWND,ctypes.POINTER(wt.RECT)];user.GetWindowRect.restype=wt.BOOL
    user.GetWindowLongPtrW.argtypes=[wt.HWND,ctypes.c_int];user.GetWindowLongPtrW.restype=ctypes.c_ssize_t
    user.GetLayeredWindowAttributes.argtypes=[wt.HWND,ctypes.POINTER(wt.DWORD),ctypes.POINTER(wt.BYTE),ctypes.POINTER(wt.DWORD)];user.GetLayeredWindowAttributes.restype=wt.BOOL
    user.SendMessageW.argtypes=[wt.HWND,wt.UINT,wt.WPARAM,wt.LPARAM];user.SendMessageW.restype=ctypes.c_ssize_t
    callback_type=ctypes.WINFUNCTYPE(wt.BOOL,wt.HWND,wt.LPARAM)
    user.EnumChildWindows.argtypes=[wt.HWND,callback_type,wt.LPARAM]
    def owner_window(pid):
        hwnd=user.FindWindowW('WuWaPCControlPanel',None)
        actual=wt.DWORD();user.GetWindowThreadProcessId(hwnd,ctypes.byref(actual))
        return hwnd if hwnd and actual.value==pid else None
    def hotkey_field(hwnd,class_name='msctls_hotkey32'):
        fields=[]
        @callback_type
        def collect(child,_):
            name=ctypes.create_unicode_buffer(128);user.GetClassNameW(child,name,128)
            if name.value.lower()==class_name.lower():fields.append(child)
            return True
        user.EnumChildWindows(hwnd,collect,0)
        return fields[0] if fields else None
    with tempfile.TemporaryDirectory(prefix='wuwa-windows-') as directory:
        # Isolated settings; never read or replace a developer/user's config.
        env=os.environ.copy();env['LOCALAPPDATA']=directory
        settings=Path(directory)/'WuWaPC/settings.ini';settings.parent.mkdir()
        settings.write_text('[Keybinds]\nMenu=135:6\n') # Ctrl+Shift+F24
        for run in range(2):
            child=subprocess.Popen([str(exe)],env=env)
            try:
                hwnd=wait_until(lambda:owner_window(child.pid),'No app window')
                time.sleep(.8)
                assert child.poll() is None and not user.IsWindowVisible(hwnd),'App did not start hidden'
                assert user.PostMessageW(hwnd,0x8002,0,0) # app's own Show command; no keyboard input
                wait_until(lambda:user.IsWindowVisible(hwnd),'Show command did not open menu')
                field=wait_until(lambda:hotkey_field(hwnd),'Hotkey field missing')
                assert not user.IsWindowVisible(field),'Hidden key tab leaked a control'
                if run==0:
                    body=hotkey_field(hwnd,'WuWaPCContent');assert body
                    opacity=user.GetDlgItem(body,206);footer=user.GetDlgItem(hwnd,205);assert opacity and footer
                    def rect(control):
                        value=wt.RECT();assert user.GetWindowRect(control,ctypes.byref(value));return value.left,value.top,value.right,value.bottom
                    before=rect(opacity);footer_before=rect(footer)
                    user.SendMessageW(body,0x020a,((-120)&0xffff)<<16,0)
                    assert rect(opacity)[1]<before[1],'Content did not scroll'
                    assert rect(footer)==footer_before,'Footer moved with scrolling content'
                    user.SendMessageW(body,0x020a,1440<<16,0)
                    assert rect(opacity)==before,'Scroll did not return to the top'
                    capture(hwnd,ROOT/'dist/ui-preview-overview.png')
                user.SendMessageW(hwnd,0x0111,201,0) # Keybind tab
                wait_until(lambda:user.IsWindowVisible(field),'Keybind tab did not show field')
                actual=user.SendMessageW(field,0x0402,0,0) # HKM_GETHOTKEY
                expected=135 if run==0 else 134
                assert actual&255==expected,'Saved custom key did not reload'
                if run==0:
                    user.SendMessageW(field,0x0401,134|(2|4)<<8,0) # Ctrl+Alt+F23
                    user.SendMessageW(hwnd,0x0111,101,0) # Save
                    wait_until(lambda:'134:3' in settings.read_text(),'Custom key was not persisted')
                    capture(hwnd,ROOT/'dist/ui-preview-keybinds.png')
                    assert user.GetWindowLongPtrW(hwnd,-20)&8 # WS_EX_TOPMOST
                    user.SendMessageW(hwnd,0x0111,202,0) # Unpin
                    assert not user.GetWindowLongPtrW(hwnd,-20)&8
                    user.SendMessageW(hwnd,0x0111,220,0) # 85% opacity
                    color=wt.DWORD();alpha=wt.BYTE();flags=wt.DWORD()
                    assert user.GetLayeredWindowAttributes(hwnd,ctypes.byref(color),ctypes.byref(alpha),ctypes.byref(flags))
                    assert alpha.value==255*85//100
                    user.SendMessageW(hwnd,0x0111,211,0) # English
                    capture(hwnd,ROOT/'dist/ui-preview-english.png')
                    user.SendMessageW(hwnd,0x0111,200,0) # Overview
                    assert not user.IsWindowVisible(field)
                else:
                    assert not user.GetWindowLongPtrW(hwnd,-20)&8,'Pin preference did not reload'
                    raw=settings.read_text();assert 'Opacity=85' in raw and 'Language=en' in raw
                user.SendMessageW(hwnd,0x0111,204,0) # Footer Change shortcut
                assert user.IsWindowVisible(field)
                user.SendMessageW(hwnd,0x0111,200,0) # Hide editing field and restore shortcut registration
                assert not user.IsWindowVisible(field)
                assert user.PostMessageW(hwnd,0x0010,0,0) # X hides, not exits
                wait_until(lambda:not user.IsWindowVisible(hwnd),'Close did not hide')
                assert child.poll() is None
                assert user.PostMessageW(hwnd,0x0111,103,0) # explicit Exit
                assert child.wait(timeout=10)==0
            finally:
                if child.poll() is None:child.terminate();child.wait(timeout=5)
    print('PASS actual EXE startup hidden, show/hide/exit, custom-key Save and persistence across restart; no real game or input')
def main(zig):
    if os.name!='nt':raise SystemExit('This test requires Windows')
    exe=ROOT/'dist/WuWa-PC.exe'
    subprocess.run([str(exe),'--self-test'],check=True,timeout=30)
    with tempfile.TemporaryDirectory(prefix='wuwa-native-') as directory:
        fixture=Path(directory)/'Thử nghiệm 游戏'/'Client-Win64-Shipping.exe'
        fixture.parent.mkdir()
        subprocess.run([zig,'cc','-target','x86_64-windows-gnu','-std=c11','-O2','-Wall','-Wextra','-Werror','-municode','-Wl,--subsystem,windows',str(ROOT/'tests/launcher_fixture.c'),'-luser32','-o',str(fixture)],check=True)
        fixture_env=os.environ.copy();fixture_env['WUWA_TEST_GAME']=str(fixture)
        for name,libs in [('core_test',[]),('windows_test',['-luser32','-lgdi32','-lshell32','-lcomctl32','-lcomdlg32'])]:
            out=Path(directory)/(name+'.exe')
            subprocess.run([zig,'cc','-target','x86_64-windows-gnu','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-UNDEBUG',str(ROOT/'tests'/f'{name}.c'),*libs,'-o',str(out)],check=True)
            subprocess.run([str(out)],check=True,timeout=45,env=fixture_env)
    lifecycle(exe)
    report={'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'commit':os.environ.get('GITHUB_SHA'),'verification_url':f"https://github.com/{os.environ.get('GITHUB_REPOSITORY')}/actions/runs/{os.environ.get('GITHUB_RUN_ID')}",'windows_runtime_tested':True,'game_runtime_tested':False,'tests':['packaged_exe_self_test','native_core_and_key_transaction','native_controls_conflict_save_rollback_and_geometry','actual_exe_hidden_start_show_hide_exit_and_key_persistence','reference_style_tabs_language_pin_opacity_and_persistence','own_app_ui_previews_no_game_capture','unicode_game_path_persistence_and_corrupt_config','synthetic_game_launch_wait_duplicate_guard_auto_hide_and_timeout']}
    (ROOT/'dist/windows-verification.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--zig',default='zig');main(p.parse_args().zig)
