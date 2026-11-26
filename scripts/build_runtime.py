"""Build our live controller/bridge; does not use or modify game binaries."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / 'dist'


def main(zig, dotnet):
    subprocess.run([dotnet, 'build', str(ROOT / 'runtime/managed/Sora.Runtime.csproj'), '-c', 'Release', '--nologo'], check=True)
    subprocess.run([dotnet, 'run', '--project', str(ROOT / 'runtime/tests/Runtime.Tests.csproj'), '-c', 'Release', '--nologo'], check=True)
    DIST.mkdir(exist_ok=True)
    for name in ('Sora.Runtime.dll', '0Harmony.dll'):
        shutil.copyfile(ROOT / 'runtime/managed/bin/Release/net472' / name, DIST / name)
    subprocess.run([zig, 'cc', '-target', 'x86_64-windows-gnu', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-shared', str(ROOT / 'runtime/native/bridge.c'), '-ladvapi32', '-o', str(DIST / 'Sora.Runtime.Native.dll')], check=True)
    if os.name != 'nt':
        print('PASS managed hook fixture and Windows bridge compilation; freeze/remote-load test requires Windows')
        return
    subprocess.run([sys.executable, '-m', 'PyInstaller', '--noconfirm', '--clean', '--onefile', '--console',
                    '--name', 'Sora-Live', '--distpath', str(DIST), '--workpath', str(DIST / 'live-build'),
                    '--specpath', str(DIST), str(ROOT / 'runtime/controller/live.py')], check=True)
    assets = json.loads((ROOT / 'runtime/managed/obj/project.assets.json').read_text())
    licenses = [Path(folder) / 'lib.harmony/2.4.2/LICENSE' for folder in assets['packageFolders']]
    license_text = next(path.read_text(encoding='utf-8') for path in licenses if path.is_file())
    if 'MIT License' not in license_text: raise RuntimeError('Unexpected dependency license')
    (DIST / 'Harmony-LICENSE.txt').write_text(license_text, encoding='utf-8')
    report = {'feature': 'live_no_cooldown', 'game_runtime_tested': False,
              'method_detours_tested_in_fixture': True,
              'loaded_mono_exports_required': True,
              'server_effects_verified': False,
              'files': {name: hashlib.sha256((DIST / name).read_bytes()).hexdigest()
                        for name in ('Sora-Live.exe', 'Sora.Runtime.Native.dll', 'Sora.Runtime.dll', '0Harmony.dll')}}
    (DIST / 'live-build.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zig', default='zig'); parser.add_argument('--dotnet', default='dotnet')
    args = parser.parse_args(); main(args.zig, args.dotnet)
