"""Prepare/remove an owner-controlled experimental PAK. No process injection."""
import argparse
import configparser
import ctypes
import hashlib
import json
import os
from pathlib import Path
import sys
import tempfile

from pak_format import build, read_image
from prepare import SOURCE_SHA256, prepare

PAK_NAME = 'zz_SoraWW_Experimental_P.pak'
PAK18_SHA256 = 'b8ed535f6d741e198e304f998a0f39198503e0b857d86315446ea442d3ea3a13'


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def destination(exe):
    if exe.name.lower() != 'client-win64-shipping.exe' or exe.parent.name.lower() != 'win64' or exe.parent.parent.name.lower() != 'binaries':
        raise ValueError('INVALID_GAME_PATH')
    return exe.parent.parent.parent / 'Content' / 'Paks' / PAK_NAME


def game_running():
    if os.name != 'nt':
        return False
    from ctypes import wintypes as w
    class Entry(ctypes.Structure):
        _fields_ = [('dwSize', w.DWORD), ('cntUsage', w.DWORD), ('th32ProcessID', w.DWORD), ('th32DefaultHeapID', ctypes.c_size_t), ('th32ModuleID', w.DWORD), ('cntThreads', w.DWORD), ('th32ParentProcessID', w.DWORD), ('pcPriClassBase', w.LONG), ('dwFlags', w.DWORD), ('szExeFile', w.WCHAR * 260)]
    k = ctypes.WinDLL('kernel32', use_last_error=True)
    k.CreateToolhelp32Snapshot.argtypes = [w.DWORD, w.DWORD]; k.CreateToolhelp32Snapshot.restype = w.HANDLE
    k.Process32FirstW.argtypes = [w.HANDLE, ctypes.POINTER(Entry)]; k.Process32FirstW.restype = w.BOOL
    k.Process32NextW.argtypes = [w.HANDLE, ctypes.POINTER(Entry)]; k.Process32NextW.restype = w.BOOL
    k.CloseHandle.argtypes = [w.HANDLE]
    handle = k.CreateToolhelp32Snapshot(2, 0)
    if handle == ctypes.c_void_p(-1).value:
        raise ValueError('PROCESS_CHECK_FAILED')
    try:
        entry = Entry(); entry.dwSize = ctypes.sizeof(entry)
        if not k.Process32FirstW(handle, ctypes.byref(entry)):
            raise ValueError('PROCESS_CHECK_FAILED')
        while True:
            if entry.szExeFile.lower() == 'client-win64-shipping.exe':
                return True
            if not k.Process32NextW(handle, ctypes.byref(entry)):
                if ctypes.get_last_error() != 18:
                    raise ValueError('PROCESS_CHECK_FAILED')
                return False
    finally:
        k.CloseHandle(handle)


def atomic_write(path, raw):
    handle, temp = tempfile.mkstemp(prefix='.sora-', suffix='.tmp', dir=path.parent)
    try:
        with os.fdopen(handle, 'wb') as stream:
            stream.write(raw); stream.flush(); os.fsync(stream.fileno())
        os.replace(temp, path)
    finally:
        if os.path.exists(temp):
            os.unlink(temp)


def read_manifest(path):
    if not path.exists():
        return None
    obj = json.loads(path.read_text())
    if obj.get('schema') != 'SORA-WW-OWNED-TRIAL-1' or not isinstance(obj.get('sha256'), str) or len(obj['sha256']) != 64:
        raise ValueError('INVALID_TRIAL_STATE')
    if str(destination(Path(obj['exe']))) != obj.get('destination'):
        raise ValueError('INVALID_TRIAL_STATE')
    return obj


def checked_owned_file(path, manifest):
    if not path.exists():
        return
    if not manifest or str(path) != manifest['destination'] or digest(path.read_bytes()) not in {manifest['sha256'], manifest.get('previous_sha256')}:
        raise ValueError('UNOWNED_OR_CHANGED_PACKAGE')


def apply_trial(exe, state, features, running=game_running):
    if running():
        raise ValueError('CLOSE_GAME_FIRST')
    manifest_path = state / 'gameplay-trial.json'
    manifest = read_manifest(manifest_path)
    target = destination(exe)
    if not features:
        if manifest:
            old = Path(manifest['destination']); checked_owned_file(old, manifest)
            if running():
                raise ValueError('CLOSE_GAME_FIRST')
            if old.exists():
                old.unlink()
            manifest_path.unlink()
        elif target.exists():
            raise ValueError('UNOWNED_OR_CHANGED_PACKAGE')
        return 'REMOVED'
    if manifest and manifest['destination'] != str(target):
        raise ValueError('OTHER_INSTALL_ACTIVE')
    checked_owned_file(target, manifest)
    source = target.parent / 'pakchunk18-WindowsNoEditor.pak'
    if source.stat().st_size != 105427118:
        raise ValueError('UNSUPPORTED_BUILD')
    raw = source.read_bytes()
    if digest(raw) != PAK18_SHA256:
        raise ValueError('UNSUPPORTED_BUILD')
    image = raw[868416:868416 + 54166016]
    if digest(image) != SOURCE_SHA256:
        raise ValueError('UNSUPPORTED_ASSEMBLY')
    patched, edits = prepare(image, features)
    package = build(patched)
    assert read_image(package) == patched
    if running():
        raise ValueError('CLOSE_GAME_FIRST')
    # Re-check immediately before replacing only our own previously recorded output.
    checked_owned_file(target, manifest)
    new_manifest = {'schema': 'SORA-WW-OWNED-TRIAL-1', 'exe': str(exe), 'destination': str(target),
                    'sha256': digest(package), 'previous_sha256': manifest['sha256'] if manifest else None, 'source_pak_sha256': PAK18_SHA256,
                    'features': sorted(set(features)), 'methods': edits,
                    'game_runtime_verified': False}
    # A recorded intended hash also permits cleanup after interruption before the PAK commit.
    atomic_write(manifest_path, (json.dumps(new_manifest, indent=2) + '\n').encode())
    try:
        atomic_write(target, package)
    except Exception:
        if manifest:
            atomic_write(manifest_path, (json.dumps(manifest, indent=2) + '\n').encode())
        elif manifest_path.exists():
            manifest_path.unlink()
        raise
    return 'PREPARED_UNVERIFIED'


def write_result(state, code):
    ini = configparser.ConfigParser(); ini['Trial'] = {'Code': code}
    import io
    text = io.StringIO(); ini.write(text)
    atomic_write(state / 'gameplay-result.ini', text.getvalue().encode('ascii'))


def self_test():
    import unittest
    import test_guard
    import test_trial
    suite = unittest.TestSuite([unittest.defaultTestLoader.loadTestsFromModule(m) for m in (test_guard, test_trial)])
    if not unittest.TextTestRunner().run(suite).wasSuccessful():
        raise SystemExit(1)
    print('PASS guarded CIL, PAK round trip/corruption and owned-output rollback checks')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--self-test', action='store_true')
    parser.add_argument('--exe', type=Path)
    parser.add_argument('--state', type=Path)
    parser.add_argument('--feature', action='append', choices=['no-cooldown', 'ignore-local-damage'], default=[])
    args = parser.parse_args()
    if args.self_test:
        self_test(); return
    if args.exe is None or args.state is None:
        parser.error('--exe and --state are required')
    state = args.state.resolve(); state.mkdir(parents=True, exist_ok=True)
    try:
        code = apply_trial(args.exe.resolve(), state, args.feature)
        write_result(state, code)
    except Exception as error:
        # Report fixed error identifiers only; never write raw game/account paths.
        known = {'CLOSE_GAME_FIRST', 'INVALID_GAME_PATH', 'UNSUPPORTED_BUILD', 'UNSUPPORTED_ASSEMBLY', 'INVALID_TRIAL_STATE', 'UNOWNED_OR_CHANGED_PACKAGE', 'OTHER_INSTALL_ACTIVE', 'PROCESS_CHECK_FAILED'}
        code = str(error) if str(error) in known else 'FILE_OR_PREPARATION_ERROR'
        write_result(state, code)
        raise SystemExit(1)


if __name__ == '__main__':
    main()
