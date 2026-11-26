from pathlib import Path
import subprocess,tempfile,sys
ROOT=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='wuwa-core-') as directory:
    binary=Path(directory)/'core-test'
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-Wno-unused-function','-fsanitize=address,undefined','-g',str(ROOT/'tests/core_test.c'),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
source=(ROOT/'src/main.c').read_text()
for api in ['ReadProcessMemory','WriteProcessMemory','CreateRemoteThread','VirtualAllocEx','SendInput','keybd_event','mouse_event','SetWindowsHookEx']:
    assert api not in source,api
assert 'GetWindowThreadProcessId' in source and 'PROCESS_QUERY_LIMITED_INFORMATION' in source
print('PASS foundation does not include gameplay-memory or input APIs')
