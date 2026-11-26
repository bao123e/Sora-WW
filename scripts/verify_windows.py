"""Verify native controls and actual packaged app lifecycle without a real game."""
from pathlib import Path
import argparse,ctypes,ctypes.wintypes as wt,hashlib,json,os,subprocess,tempfile,time
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
    user.SendMessageW.argtypes=[wt.HWND,wt.UINT,wt.WPARAM,wt.LPARAM];user.SendMessageW.restype=ctypes.c_ssize_t
    callback_type=ctypes.WINFUNCTYPE(wt.BOOL,wt.HWND,wt.LPARAM)
    user.EnumChildWindows.argtypes=[wt.HWND,callback_type,wt.LPARAM]
    def owner_window(pid):
        hwnd=user.FindWindowW('WuWaPCControlPanel',None)
        actual=wt.DWORD();user.GetWindowThreadProcessId(hwnd,ctypes.byref(actual))
        return hwnd if hwnd and actual.value==pid else None
    def hotkey_field(hwnd):
        fields=[]
        @callback_type
        def collect(child,_):
            name=ctypes.create_unicode_buffer(128);user.GetClassNameW(child,name,128)
            if name.value.lower()=='msctls_hotkey32':fields.append(child)
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
                actual=user.SendMessageW(field,0x0402,0,0) # HKM_GETHOTKEY
                expected=135 if run==0 else 134
                assert actual&255==expected,'Saved custom key did not reload'
                if run==0:
                    user.SendMessageW(field,0x0401,134|(2|4)<<8,0) # Ctrl+Alt+F23
                    user.SendMessageW(hwnd,0x0111,101,0) # Save
                    wait_until(lambda:'134:3' in settings.read_text(),'Custom key was not persisted')
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
        for name,libs in [('core_test',[]),('windows_test',['-luser32','-lgdi32','-lshell32','-lcomctl32'])]:
            out=Path(directory)/(name+'.exe')
            subprocess.run([zig,'cc','-target','x86_64-windows-gnu','-std=c11','-O2','-Wall','-Wextra','-Werror','-Wno-unused-function','-UNDEBUG',str(ROOT/'tests'/f'{name}.c'),*libs,'-o',str(out)],check=True)
            subprocess.run([str(out)],check=True,timeout=45)
    lifecycle(exe)
    report={'exe_sha256':hashlib.sha256(exe.read_bytes()).hexdigest(),'commit':os.environ.get('GITHUB_SHA'),'verification_url':f"https://github.com/{os.environ.get('GITHUB_REPOSITORY')}/actions/runs/{os.environ.get('GITHUB_RUN_ID')}",'windows_runtime_tested':True,'game_runtime_tested':False,'tests':['packaged_exe_self_test','native_core_and_key_transaction','native_controls_conflict_save_rollback_and_geometry','actual_exe_hidden_start_show_hide_exit_and_key_persistence']}
    (ROOT/'dist/windows-verification.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--zig',default='zig');main(p.parse_args().zig)
