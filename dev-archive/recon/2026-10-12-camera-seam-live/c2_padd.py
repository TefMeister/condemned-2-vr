"""One long-lived virtual pad for a game session (2026-10-12). A vanishing pad pauses these games, so the pad
lives in this process and takes commands from a text file: each line is a step like c2_drive.py's
(START, A, B, UP, DOWN, LEFT, RIGHT, waitN, shot, hold=SECONDS). The file is deleted once read.
Usage: python c2_padd.py <exe name> ; stop with a line 'quit'."""
import os, subprocess, sys, time
sys.path.insert(0, r"D:\claude video game stuff\github-backups\the-darkness-vr\dev-archive\build-scripts")
from winshot import find_window, grab  # noqa: E402
from PIL import Image  # noqa: E402
import ctypes, vgamepad as vg  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
CMD = os.path.join(HERE, "padcmd.txt")
OUT = os.path.join(HERE, "c2")
os.makedirs(OUT, exist_ok=True)
V = vg.XUSB_BUTTON
B = {"A": V.XUSB_GAMEPAD_A, "B": V.XUSB_GAMEPAD_B, "START": V.XUSB_GAMEPAD_START, "UP": V.XUSB_GAMEPAD_DPAD_UP,
     "DOWN": V.XUSB_GAMEPAD_DPAD_DOWN, "LEFT": V.XUSB_GAMEPAD_DPAD_LEFT, "RIGHT": V.XUSB_GAMEPAD_DPAD_RIGHT}
exe = sys.argv[1]
pid = int(subprocess.check_output(f'tasklist /FI "IMAGENAME eq {exe}" /FO CSV /NH', shell=True).decode().split(",")[1].strip('"'))
h = find_window(pid)
pad = vg.VX360Gamepad(); time.sleep(1.5)
hold = 0.35
n = 0
print("pad ready", flush=True)
while True:
    if not os.path.exists(CMD):
        time.sleep(0.3); continue
    steps = open(CMD).read().split(); os.remove(CMD)
    for s in steps:
        if s == "quit":
            sys.exit(0)
        if s.startswith("hold="):
            hold = float(s[5:])
        elif s.startswith("wait"):
            time.sleep(float(s[4:]))
        elif s == "shot":
            n += 1; Image.fromarray(grab(h)[:, :, :3]).save(os.path.join(OUT, f"p{n:03d}.png")); print("shot", f"p{n:03d}", flush=True)
        elif s in B:
            ctypes.windll.user32.SetForegroundWindow(h); time.sleep(0.4)
            pad.press_button(B[s]); pad.update(); time.sleep(hold); pad.release_button(B[s]); pad.update()
            print("press", s, flush=True)
        elif s.startswith("RS="):        # right stick x,y for look tests: RS=0.5,0
            x, y = (float(v) for v in s[3:].split(","))
            pad.right_joystick_float(x_value_float=x, y_value_float=y); pad.update(); print("rs", x, y, flush=True)
