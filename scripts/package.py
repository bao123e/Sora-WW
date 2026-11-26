from pathlib import Path
from zipfile import ZipFile,ZIP_DEFLATED
import hashlib,json,os
ROOT=Path(__file__).resolve().parents[1];DIST=ROOT/'dist'
metadata=json.loads((DIST/'build.json').read_text());binary=DIST/'WuWa-PC.exe'
assert hashlib.sha256(binary.read_bytes()).hexdigest()==metadata['sha256']
verification=DIST/'windows-verification.json'
if os.name=='nt':
    assert verification.exists(),'Run scripts/verify_windows.py before publishing Windows builds'
if verification.exists():
    report=json.loads(verification.read_text());assert report['exe_sha256']==metadata['sha256']
    assert report['windows_runtime_tested'] and not report['game_runtime_tested']
    metadata['windows_runtime_tested']=True;metadata['windows_verification']=report
    (DIST/'build.json').write_text(json.dumps(metadata,indent=2)+'\n')
for name,digest in metadata['source_sha256'].items():assert hashlib.sha256((ROOT/name).read_bytes()).hexdigest()==digest
customer=DIST/('WuWa-PC-'+metadata['version']+'-win64.zip')
with ZipFile(customer,'w',ZIP_DEFLATED,compresslevel=9) as z:
    z.write(binary,'WuWa-PC.exe');z.write(DIST/'build.json','build.json')
    for name in ['Start-WuWa-PC.cmd','Check-Windows.cmd','LICENSE']:z.write(ROOT/name,name)
    z.write(ROOT/'README.vi.md','README.vi.txt')
    z.writestr('SHA256SUMS.txt',metadata['sha256']+'  WuWa-PC.exe\n')
source=DIST/('WuWa-PC-'+metadata['version']+'-source.zip')
with ZipFile(source,'w',ZIP_DEFLATED,compresslevel=9) as z:
    for directory in ['src','scripts','tests','.github']:
        for path in sorted((ROOT/directory).rglob('*')):
            if path.is_file() and '__pycache__' not in path.parts:z.write(path,str(path.relative_to(ROOT)))
    for name in ['README.md','README.vi.md','LICENSE','Start-WuWa-PC.cmd','Check-Windows.cmd','.gitignore','.gitattributes']:z.write(ROOT/name,name)
for archive in [customer,source]:
    with ZipFile(archive) as z:assert z.testzip() is None
    Path(str(archive)+'.sha256').write_text(hashlib.sha256(archive.read_bytes()).hexdigest()+'  '+archive.name+'\n')
    print(archive)
with ZipFile(customer) as z:
    assert hashlib.sha256(z.read('WuWa-PC.exe')).hexdigest()==json.loads(z.read('build.json'))['sha256']
    assert not json.loads(z.read('build.json'))['gameplay_implemented']
    assert not any(name.endswith(('.pem','.dll','.env')) for name in z.namelist())
print('PASS packaged executable hash, source identity, metadata and complete ZIP contents')
