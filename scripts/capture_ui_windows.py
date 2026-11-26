"""Capture only the verified test application's own window; CI use, no game capture."""
from pathlib import Path
import ctypes,ctypes.wintypes as wt,struct,zlib

def capture(hwnd,path):
    user=ctypes.WinDLL('user32',use_last_error=True);gdi=ctypes.WinDLL('gdi32',use_last_error=True)
    class Header(ctypes.Structure):
        _fields_=[('size',wt.DWORD),('width',wt.LONG),('height',wt.LONG),('planes',wt.WORD),('bits',wt.WORD),('compression',wt.DWORD),('image_size',wt.DWORD),('xpels',wt.LONG),('ypels',wt.LONG),('used',wt.DWORD),('important',wt.DWORD)]
    class Info(ctypes.Structure):_fields_=[('header',Header),('colors',wt.DWORD*3)]
    user.GetWindowRect.argtypes=[wt.HWND,ctypes.POINTER(wt.RECT)];user.GetWindowRect.restype=wt.BOOL
    user.GetDC.argtypes=[wt.HWND];user.GetDC.restype=wt.HDC
    user.ReleaseDC.argtypes=[wt.HWND,wt.HDC]
    user.PrintWindow.argtypes=[wt.HWND,wt.HDC,wt.UINT];user.PrintWindow.restype=wt.BOOL
    gdi.CreateCompatibleDC.argtypes=[wt.HDC];gdi.CreateCompatibleDC.restype=wt.HDC
    gdi.CreateDIBSection.argtypes=[wt.HDC,ctypes.POINTER(Info),wt.UINT,ctypes.POINTER(ctypes.c_void_p),wt.HANDLE,wt.DWORD];gdi.CreateDIBSection.restype=wt.HBITMAP
    gdi.SelectObject.argtypes=[wt.HDC,wt.HGDIOBJ];gdi.SelectObject.restype=wt.HGDIOBJ
    gdi.DeleteObject.argtypes=[wt.HGDIOBJ];gdi.DeleteDC.argtypes=[wt.HDC];gdi.GdiFlush.argtypes=[]
    rect=wt.RECT();assert user.GetWindowRect(hwnd,ctypes.byref(rect));width,height=rect.right-rect.left,rect.bottom-rect.top
    assert 300<=width<=1600 and 300<=height<=1600
    dc=user.GetDC(hwnd);memory=gdi.CreateCompatibleDC(dc);info=Info();info.header=Header(ctypes.sizeof(Header),width,-height,1,32,0,0,0,0,0,0)
    pixels=ctypes.c_void_p();bitmap=gdi.CreateDIBSection(dc,ctypes.byref(info),0,ctypes.byref(pixels),None,0);assert bitmap and pixels.value
    previous=gdi.SelectObject(memory,bitmap)
    try:
        assert user.PrintWindow(hwnd,memory,0),'Own-window PrintWindow failed'
        gdi.GdiFlush();raw=ctypes.string_at(pixels.value,width*height*4);rows=[]
        for y in range(height):
            scan=bytearray()
            for x in range(width):
                offset=(y*width+x)*4;b,g,r,_=raw[offset:offset+4];scan.extend((r,g,b))
            rows.append(b'\0'+bytes(scan))
        assert len(set(raw[::4]))>8,'UI capture looks blank'
        def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
        png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',width,height,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(rows)))+chunk(b'IEND',b'')
        Path(path).write_bytes(png)
    finally:
        gdi.SelectObject(memory,previous);gdi.DeleteObject(bitmap);gdi.DeleteDC(memory);user.ReleaseDC(hwnd,dc)
