import ctypes, ctypes.wintypes as wt, time, subprocess
u=ctypes.windll.user32
P=ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
def cb(h,_):
    n=u.GetWindowTextLengthW(h)
    if u.IsWindowVisible(h) and n:
        b=ctypes.create_unicode_buffer(n+1); u.GetWindowTextW(h,b,n+1)
        if b.value.startswith("condemned2recomp"): u.PostMessageW(h,0x10,0,0)
    return True
u.EnumWindows(P(cb),0); time.sleep(3)
print("still running" if b"condemned2recomp" in subprocess.check_output("tasklist") else "closed")
