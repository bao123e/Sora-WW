"""Build a standalone Windows x64 menu/window foundation with Zig 0.14.1."""
from pathlib import Path
import argparse,subprocess,hashlib,json,struct
ROOT=Path(__file__).resolve().parents[1]
VERSION='0.2.0-ui-preview'
def inspect_pe(path):
    data=path.read_bytes();pe=struct.unpack_from('<I',data,0x3c)[0]
    assert data[:2]==b'MZ' and data[pe:pe+4]==b'PE\0\0'
    assert struct.unpack_from('<H',data,pe+4)[0]==0x8664
    optional=pe+24;assert struct.unpack_from('<H',data,optional)[0]==0x20b
    assert struct.unpack_from('<H',data,optional+68)[0]==2  # Windows GUI
    count=struct.unpack_from('<H',data,pe+6)[0];size=struct.unpack_from('<H',data,pe+20)[0]
    sections=[]
    for n in range(count):
        off=optional+size+40*n
        virtual_size,rva,raw_size,raw=struct.unpack_from('<IIII',data,off+8)
        sections.append((rva,max(virtual_size,raw_size),raw))
    def offset(rva):
        for start,length,raw in sections:
            if start<=rva<start+length:return raw+rva-start
        raise ValueError('RVA outside sections')
    def string(off):return data[off:data.index(b'\0',off)].decode('ascii')
    import_rva=struct.unpack_from('<I',data,optional+120)[0];imports=[];dlls=[]
    descriptor=offset(import_rva)
    while any(data[descriptor:descriptor+20]):
        original,_,_,name,thunk=struct.unpack_from('<IIIII',data,descriptor)
        dlls.append(string(offset(name)))
        table=offset(original or thunk)
        while (value:=struct.unpack_from('<Q',data,table)[0]):
            if not value&(1<<63):imports.append(string(offset(value)+2))
            table+=8
        descriptor+=20
    forbidden={'ReadProcessMemory','WriteProcessMemory','VirtualAllocEx','CreateRemoteThread','SendInput','keybd_event','mouse_event','SetWindowsHookExW','SetWindowsHookExA'}
    assert not forbidden.intersection(imports), 'Unexpected game/input API import'
    assert 'QueryFullProcessImageNameW' in imports and 'Shell_NotifyIconW' in imports
    return {'architecture':'AMD64','subsystem':'Windows GUI','dlls':sorted(dlls),'imports':sorted(imports),'forbidden_game_input_apis_absent':True}
def build(zig):
    out=ROOT/'dist';out.mkdir(exist_ok=True)
    resource=out/'app.rc';resource.write_text('1 24 '+json.dumps((ROOT/'src/app.manifest').as_posix())+'\n')
    subprocess.run([zig,'rc','/fo',str(out/'app.res'),'--',str(resource)],check=True)
    command=[zig,'cc','-target','x86_64-windows-gnu','-std=c11','-O2','-g0','-Wall','-Wextra','-Werror','-Wno-unused-function','-municode','-Wl,--subsystem,windows',str(ROOT/'src/main.c'),str(out/'app.res'),'-luser32','-lgdi32','-lshell32','-lcomctl32','-o',str(out/'WuWa-PC.exe')]
    subprocess.run(command,check=True)
    image=out/'WuWa-PC.exe';result=inspect_pe(image)
    metadata={'version':VERSION,'stage':'menu-window-ui-preview','ui_reference':'owner-provided MFG Unlocker screenshot; styling only','ui_languages':['vi','en'],'gameplay_implemented':False,'windows_runtime_tested':False,'game_runtime_tested':False,'observed_game_version':'OS_Windows_Product_3.8.0_9244589_3.8.13_9534983 * SD','observation_source':'owner-provided title-screen screenshot 2026-11-26; not an executable compatibility check','source_sha256':{name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ['src/main.c','src/core.h','src/ui.h','src/app.manifest']},'sha256':hashlib.sha256(image.read_bytes()).hexdigest(),'pe':result}
    (out/'build.json').write_text(json.dumps(metadata,indent=2)+'\n')
    print('PASS Windows x64 GUI build and PE imports; live Windows/game UI still unverified')
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--zig',default='zig');args=parser.parse_args();build(args.zig)
