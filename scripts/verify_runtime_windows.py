"""Exercise the real native loader/pipe against only our synthetic EXE."""
from pathlib import Path
import argparse
import importlib.util
import json
import subprocess
import tempfile
import time
import ctypes as c

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('sora_live', ROOT / 'runtime/controller/live.py')
live = importlib.util.module_from_spec(spec); spec.loader.exec_module(live)


def main(zig):
    api = live.Windows()
    with tempfile.TemporaryDirectory(prefix='sora-live-fixture-') as directory:
        folder = Path(directory)
        host = folder / 'Client-Win64-Shipping.exe'
        subprocess.run([zig, 'cc', '-target', 'x86_64-windows-gnu', '-municode', '-Wl,--subsystem,windows',
                        str(ROOT / 'tests/launcher_fixture.c'), '-luser32', '-o', str(host)], check=True)
        child = subprocess.Popen([str(host), '-krqlv=sd'], cwd=folder)
        pipe = None
        try:
            time.sleep(1)
            assert child.poll() is None
            bridge = (ROOT / 'dist/Sora.Runtime.Native.dll').resolve()
            api.inject(child.pid, bridge)
            pipe = api.pipe(child.pid)
            assert api.command(pipe, 'OFF') == 'OFF'
            try:
                api.command(pipe, 'ON')
            except live.RuntimeErrorCode as error:
                assert str(error) == 'MONO_EXPORTS_UNAVAILABLE', str(error)
            else: raise AssertionError('A process without Mono was falsely reported active')
            assert api.command(pipe, 'OFF') == 'OFF'
            api.CloseHandle(pipe); pipe = None
            # The bridge remains connected to no UI; reconnect is safe and the
            # injector recognizes its exact already-loaded path.
            api.inject(child.pid, bridge)
            pipe = api.pipe(child.pid)
            assert api.command(pipe, 'OFF') == 'OFF'
            assert sum(Path(path).resolve() == bridge for path, _ in api.modules(child.pid)) == 1
            assert child.poll() is None
        finally:
            if pipe: api.CloseHandle(pipe)
            child.terminate(); child.wait(timeout=10)
        live.report(folder, 73, 'MONO_EXPORTS_UNAVAILABLE')
        result = (folder / 'live-result.ini').read_text(encoding='utf-16')
        assert 'Sequence=73' in result and 'Code=MONO_EXPORTS_UNAVAILABLE' in result
    report = json.loads((ROOT / 'dist/live-build.json').read_text())
    report['windows_bridge_fixture_tested'] = True
    report['tests'] = ['remote_load_into_our_fixture', 'server_pid_verification', 'OFF_acknowledgment',
                       'reject_process_without_exported_Mono', 'reconnect_without_duplicate_load', 'sequence_ack_encoding']
    (ROOT / 'dist/live-build.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS actual Windows remote-load, secured pipe, explicit missing-Mono rejection and reconnect; no game used')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(); parser.add_argument('--zig', default='zig')
    main(parser.parse_args().zig)
