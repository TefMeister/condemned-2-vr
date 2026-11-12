import ctypes, ctypes.wintypes as wt, sys, subprocess
from PIL import Image
u=ctypes.windll.user32; g=ctypes.windll.gdi32
u.SetProcessDPIAware()
def wins():
    out=[]
    P=ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(h,_):
        n=u.GetWindowTextLengthW(h)
        if u.IsWindowVisible(h) and n:
            b=ctypes.create_unicode_buffer(n+1); u.GetWindowTextW(h,b,n+1)
            if b.value.startswith("condemned2recomp"): out.append(h)
        return True
    u.EnumWindows(P(cb),0); return out
h=wins()[0]
if "focus" in sys.argv:
    u.ShowWindow(h,9); u.keybd_event(0x12,0,0,0); u.SetForegroundWindow(h); u.keybd_event(0x12,0,2,0)
r=wt.RECT(); u.GetClientRect(h,ctypes.byref(r)); w,hh=r.right,r.bottom
dc=u.GetDC(h); m=g.CreateCompatibleDC(dc); bm=g.CreateCompatibleBitmap(dc,w,hh); g.SelectObject(m,bm)
u.PrintWindow(h,m,3)
class BI(ctypes.Structure): _fields_=[("s",wt.DWORD),("w",ctypes.c_long),("h",ctypes.c_long),("p",wt.WORD),("b",wt.WORD),("c",wt.DWORD),("si",wt.DWORD),("x",ctypes.c_long),("y",ctypes.c_long),("u",wt.DWORD),("i",wt.DWORD)]
bi=BI(40,w,-hh,1,32,0,0,0,0,0,0); buf=ctypes.create_string_buffer(w*hh*4)
g.GetDIBits(m,bm,0,hh,buf,ctypes.byref(bi),0)
im=Image.frombuffer("RGBA",(w,hh),buf,"raw","BGRA",0,1).convert("RGB")
im.thumbnail((800,800)); im.save(sys.argv[1]); print(w,hh,"fg" if u.GetForegroundWindow()==h else "not fg")
