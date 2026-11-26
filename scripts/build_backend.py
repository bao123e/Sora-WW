"""Freeze the file-based trial preparer; no game files are used by this build."""
from pathlib import Path
import hashlib
import importlib.metadata
import json
import os
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def main():
    if os.name != 'nt':
        raise SystemExit('The bundled backend must be built and self-tested on Windows.')
    folder = ROOT / 'tools/gameplay-prototype'
    subprocess.run([sys.executable, str(folder / 'trial_backend.py'), '--self-test'], check=True)
    subprocess.run([sys.executable, '-m', 'PyInstaller', '--noconfirm', '--clean', '--onefile', '--console',
                    '--name', 'Sora-Gameplay', '--paths', str(folder),
                    '--distpath', str(ROOT / 'dist'), '--workpath', str(ROOT / 'dist/backend-build'),
                    '--specpath', str(ROOT / 'dist'), str(folder / 'trial_backend.py')], check=True)
    image = ROOT / 'dist/Sora-Gameplay.exe'
    subprocess.run([str(image), '--self-test'], check=True, timeout=60)
    notices = []
    for name in ('dnfile', 'dncil', 'pefile', 'pyinstaller', 'pyinstaller-hooks-contrib', 'altgraph', 'packaging', 'pywin32-ctypes'):
        distribution = importlib.metadata.distribution(name)
        texts = []
        for item in distribution.files or []:
            if str(item.name).lower().startswith(('license', 'copying', 'notice')) or '/licenses/' in str(item).lower():
                path = distribution.locate_file(item)
                if path.is_file():
                    texts.append(path.read_text(encoding='utf-8', errors='replace'))
        if not texts:
            raise RuntimeError('License text not located for ' + name)
        notices.append(name + ' ' + distribution.version + '\n\n' + '\n\n'.join(texts))
    python_license = Path(sys.base_prefix) / 'LICENSE.txt'
    if not python_license.exists():
        raise RuntimeError('Python license text not found')
    notices.append('Python\n\n' + python_license.read_text(encoding='utf-8', errors='replace'))
    (ROOT / 'dist/THIRD_PARTY_NOTICES.txt').write_text('\n\n' + ('\n\n' + '=' * 72 + '\n\n').join(notices), encoding='utf-8')
    metadata = {'sha256': hashlib.sha256(image.read_bytes()).hexdigest(), 'windows_self_tested': True,
                'game_runtime_tested': False, 'network_or_injection_implemented': False}
    (ROOT / 'dist/backend-build.json').write_text(json.dumps(metadata, indent=2) + '\n')
    print('PASS frozen Windows backend self-test; no game installation or game binary used in CI')


if __name__ == '__main__':
    main()
