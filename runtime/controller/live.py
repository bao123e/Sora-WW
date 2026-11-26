"""Live-session controller. Original game files are never written.

Loads our bridge through ordinary Windows process rights and LoadLibraryW.
Denied rights, a missing exported Mono ABI, or a different game build are errors;
there is no protection bypass, signature patch, memory scan or guessed address.
"""
from pathlib import Path
import argparse
import ctypes as c
from ctypes import wintypes as w
import hashlib
import os
import sys
import time

BUILD = 'f648064a90fa818912ce037af9d5c3fd6b94973a415a64862ff52b3cd23e3761'
INVALID = c.c_void_p(-1).value


class RuntimeErrorCode(Exception):
    pass


class MODULE(c.Structure):
    _fields_ = [('size', w.DWORD), ('id', w.DWORD), ('pid', w.DWORD),
                ('global_use', w.DWORD), ('process_use', w.DWORD),
                ('base', c.c_void_p), ('bytes', w.DWORD), ('module', w.HMODULE),
                ('name', w.WCHAR * 256), ('path', w.WCHAR * 260)]


class MEMORY(c.Structure):
    _fields_ = [('base', c.c_void_p), ('allocation_base', c.c_void_p),
                ('protection', w.DWORD), ('partition', w.WORD),
                ('size', c.c_size_t), ('state', w.DWORD),
                ('protect', w.DWORD), ('kind', w.DWORD)]


class Windows:
    def __init__(self):
        if os.name != 'nt' or c.sizeof(c.c_void_p) != 8:
            raise RuntimeErrorCode('WINDOWS_X64_REQUIRED')
        self.k = c.WinDLL('kernel32', use_last_error=True)
        def api(name, result, *args):
            f = getattr(self.k, name); f.restype = result; f.argtypes = list(args)
            setattr(self, name, f)
        api('OpenProcess', w.HANDLE, w.DWORD, w.BOOL, w.DWORD)
        api('CloseHandle', w.BOOL, w.HANDLE)
        api('QueryFullProcessImageNameW', w.BOOL, w.HANDLE, w.DWORD, w.LPWSTR, c.POINTER(w.DWORD))
        api('CreateToolhelp32Snapshot', w.HANDLE, w.DWORD, w.DWORD)
        api('Module32FirstW', w.BOOL, w.HANDLE, c.POINTER(MODULE))
        api('Module32NextW', w.BOOL, w.HANDLE, c.POINTER(MODULE))
        api('VirtualAllocEx', c.c_void_p, w.HANDLE, c.c_void_p, c.c_size_t, w.DWORD, w.DWORD)
        api('VirtualFreeEx', w.BOOL, w.HANDLE, c.c_void_p, c.c_size_t, w.DWORD)
        api('WriteProcessMemory', w.BOOL, w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t))
        api('CreateRemoteThread', w.HANDLE, w.HANDLE, c.c_void_p, c.c_size_t, c.c_void_p, c.c_void_p, w.DWORD, c.POINTER(w.DWORD))
        api('WaitForSingleObject', w.DWORD, w.HANDLE, w.DWORD)
        api('GetModuleHandleW', w.HMODULE, w.LPCWSTR)
        api('GetProcAddress', c.c_void_p, w.HMODULE, c.c_char_p)
        api('VirtualQuery', c.c_size_t, c.c_void_p, c.POINTER(MEMORY), c.c_size_t)
        api('GetModuleFileNameW', w.DWORD, w.HMODULE, w.LPWSTR, w.DWORD)
        api('CreateFileW', w.HANDLE, w.LPCWSTR, w.DWORD, w.DWORD, c.c_void_p, w.DWORD, w.DWORD, w.HANDLE)
        api('GetNamedPipeServerProcessId', w.BOOL, w.HANDLE, c.POINTER(w.ULONG))
        api('SetNamedPipeHandleState', w.BOOL, w.HANDLE, c.POINTER(w.DWORD), c.c_void_p, c.c_void_p)
        api('ReadFile', w.BOOL, w.HANDLE, c.c_void_p, w.DWORD, c.POINTER(w.DWORD), c.c_void_p)
        api('WriteFile', w.BOOL, w.HANDLE, c.c_void_p, w.DWORD, c.POINTER(w.DWORD), c.c_void_p)
        api('GetPrivateProfileStringW', w.DWORD, w.LPCWSTR, w.LPCWSTR, w.LPCWSTR, w.LPWSTR, w.DWORD, w.LPCWSTR)
        api('IsWow64Process2', w.BOOL, w.HANDLE, c.POINTER(w.USHORT), c.POINTER(w.USHORT))

    def modules(self, pid):
        snapshot = self.CreateToolhelp32Snapshot(0x8 | 0x10, pid)
        if snapshot == INVALID: raise RuntimeErrorCode('MODULE_QUERY_DENIED')
        result = []
        try:
            item = MODULE(); item.size = c.sizeof(item)
            more = self.Module32FirstW(snapshot, c.byref(item))
            while more:
                result.append((item.path, item.base))
                more = self.Module32NextW(snapshot, c.byref(item))
        finally: self.CloseHandle(snapshot)
        return result

    def image(self, process):
        path = c.create_unicode_buffer(32768); length = w.DWORD(len(path))
        if not self.QueryFullProcessImageNameW(process, 0, path, c.byref(length)):
            raise RuntimeErrorCode('PROCESS_IDENTITY_UNAVAILABLE')
        return Path(path.value)

    def inject(self, pid, library):
        library = library.resolve(strict=True)
        if any(os.path.normcase(path) == os.path.normcase(str(library)) for path, _ in self.modules(pid)):
            return
        process = self.OpenProcess(0x2 | 0x8 | 0x10 | 0x20 | 0x400 | 0x100000, False, pid)
        if not process: raise RuntimeErrorCode('PROCESS_ACCESS_DENIED')
        remote = None; worker = None; finished = True
        try:
            machine, native = w.USHORT(), w.USHORT()
            if not self.IsWow64Process2(process, c.byref(machine), c.byref(native)) or machine.value or native.value != 0x8664:
                raise RuntimeErrorCode('UNSUPPORTED_PROCESS_ARCHITECTURE')
            # Resolve forwarded LoadLibraryW in its actual local system module,
            # then relocate by the same module's base in the remote process.
            address = self.GetProcAddress(self.GetModuleHandleW('kernel32.dll'), b'LoadLibraryW')
            region = MEMORY()
            if not address or not self.VirtualQuery(address, c.byref(region), c.sizeof(region)):
                raise RuntimeErrorCode('SYSTEM_LOADER_UNAVAILABLE')
            name = c.create_unicode_buffer(32768)
            if not self.GetModuleFileNameW(region.allocation_base, name, len(name)):
                raise RuntimeErrorCode('SYSTEM_LOADER_UNAVAILABLE')
            bases = [base for path, base in self.modules(pid) if os.path.normcase(path) == os.path.normcase(name.value)]
            if len(bases) != 1: raise RuntimeErrorCode('SYSTEM_LOADER_UNAVAILABLE')
            target = bases[0] + address - region.allocation_base
            data = str(library).encode('utf-16-le') + b'\0\0'
            remote = self.VirtualAllocEx(process, None, len(data), 0x3000, 0x04)
            if not remote: raise RuntimeErrorCode('BRIDGE_ALLOCATION_DENIED')
            count = c.c_size_t()
            if not self.WriteProcessMemory(process, remote, data, len(data), c.byref(count)) or count.value != len(data):
                raise RuntimeErrorCode('BRIDGE_PATH_WRITE_DENIED')
            worker = self.CreateRemoteThread(process, None, 0, target, remote, 0, None)
            if not worker: raise RuntimeErrorCode('BRIDGE_LOAD_DENIED')
            finished = self.WaitForSingleObject(worker, 8000) == 0
            if not finished: raise RuntimeErrorCode('BRIDGE_LOAD_TIMEOUT')
            if not any(os.path.normcase(path) == os.path.normcase(str(library)) for path, _ in self.modules(pid)):
                raise RuntimeErrorCode('BRIDGE_NOT_LOADED')
        finally:
            # A still-running loader might read this path after timeout. Do not
            # free beneath it, terminate its thread or inject a second attempt.
            if remote and finished: self.VirtualFreeEx(process, remote, 0, 0x8000)
            if worker: self.CloseHandle(worker)
            self.CloseHandle(process)

    def pipe(self, pid):
        end = time.monotonic() + 8
        while time.monotonic() < end:
            handle = self.CreateFileW(r'\\.\pipe\SoraWW.Runtime.1.' + str(pid), 0xc0000000, 0, None, 3, 0, None)
            if handle != INVALID:
                actual = w.ULONG(); mode = w.DWORD(2)
                if (not self.GetNamedPipeServerProcessId(handle, c.byref(actual)) or actual.value != pid or
                        not self.SetNamedPipeHandleState(handle, c.byref(mode), None, None)):
                    self.CloseHandle(handle); raise RuntimeErrorCode('PIPE_IDENTITY_MISMATCH')
                return handle
            time.sleep(.1)
        raise RuntimeErrorCode('BRIDGE_PIPE_UNAVAILABLE')

    def command(self, pipe, command):
        data = command.encode('ascii'); count = w.DWORD(); buffer = c.create_string_buffer(256)
        if not self.WriteFile(pipe, data, len(data), c.byref(count), None) or count.value != len(data):
            raise RuntimeErrorCode('BRIDGE_DISCONNECTED')
        if not self.ReadFile(pipe, buffer, 255, c.byref(count), None):
            raise RuntimeErrorCode('BRIDGE_DISCONNECTED')
        answer = buffer.raw[:count.value].decode('ascii')
        if answer != 'OFF' and not answer.startswith('HOOKS_READY:'):
            raise RuntimeErrorCode(answer)
        return answer

    def request(self, state):
        text = c.create_unicode_buffer(64)
        self.GetPrivateProfileStringW('Live', 'Request', '0:0', text, 64, str(state / 'live-command.ini'))
        parts = text.value.split(':')
        if len(parts) != 2 or not parts[0].isdigit() or parts[1] not in ('0', '1'):
            raise RuntimeErrorCode('INVALID_COMMAND')
        return int(parts[0]), parts[1] == '1'


def report(state, sequence, code):
    # Same directory replacement means readers never observe a half-written ACK.
    output = state / 'live-result.ini'; temporary = state / 'live-result.tmp'
    temporary.write_text(f'[Live]\nSequence={sequence}\nCode={code}\n', encoding='utf-16')
    os.replace(temporary, output)


def run(args):
    api = Windows(); state = Path(args.state).resolve(strict=True)
    sequence = 0; pipe = None; parent = None; game = None
    try:
        sequence, enabled = api.request(state)
        parent = api.OpenProcess(0x100000, False, args.parent)
        game = api.OpenProcess(0x100000 | 0x1000, False, args.pid)
        if not parent or not game: raise RuntimeErrorCode('PROCESS_ACCESS_DENIED')
        actual = api.image(game).resolve(strict=True); selected = Path(args.game).resolve(strict=True)
        if actual != selected or actual.name != 'Client-Win64-Shipping.exe':
            raise RuntimeErrorCode('PROCESS_IDENTITY_MISMATCH')
        old = actual.parent / '../../Content/Paks/zz_SoraWW_Experimental_P.pak'
        if old.exists(): raise RuntimeErrorCode('REMOVE_OLD_TRIAL_FIRST')
        with actual.open('rb') as image:
            if hashlib.file_digest(image, 'sha256').hexdigest() != BUILD:
                raise RuntimeErrorCode('UNSUPPORTED_EXE_BUILD')
        folder = Path(sys.executable).parent if getattr(sys, 'frozen', False) else Path(__file__).resolve().parents[2] / 'dist'
        for name in ('Sora.Runtime.Native.dll', 'Sora.Runtime.dll', '0Harmony.dll'):
            if not (folder / name).is_file(): raise RuntimeErrorCode('RUNTIME_FILES_MISSING')
        if not enabled: report(state, sequence, 'OFF'); return
        api.inject(args.pid, folder / 'Sora.Runtime.Native.dll')
        pipe = api.pipe(args.pid)
        while api.WaitForSingleObject(parent, 0) == 258 and api.WaitForSingleObject(game, 0) == 258:
            sequence, enabled = api.request(state)
            answer = api.command(pipe, 'ON' if enabled else 'OFF')
            report(state, sequence, answer)
            if not enabled: return
            time.sleep(.25)
        report(state, sequence, 'SESSION_ENDED')
    except RuntimeErrorCode as error:
        report(state, sequence, str(error))
    except Exception:
        report(state, sequence, 'CONTROLLER_FAILED')
    finally:
        if pipe:
            try: api.command(pipe, 'OFF')
            except Exception: pass
            api.CloseHandle(pipe)
        if parent: api.CloseHandle(parent)
        if game: api.CloseHandle(game)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', type=int, required=True)
    parser.add_argument('--game', required=True)
    parser.add_argument('--parent', type=int, required=True)
    parser.add_argument('--state', required=True)
    run(parser.parse_args())
