"""One long-lived virtual Xbox pad; commands appended as lines to padcmd.txt beside this file:
START A B X Y UP DOWN LEFT RIGHT LB RB, waitN, ls=x,y,seconds (left stick), quit."""
import os, sys, time
import vgamepad as vg
HERE = os.path.dirname(os.path.abspath(__file__)); CMD = os.path.join(HERE, "padcmd.txt")
V = vg.XUSB_BUTTON
B = {"A": V.XUSB_GAMEPAD_A, "B": V.XUSB_GAMEPAD_B, "X": V.XUSB_GAMEPAD_X, "Y": V.XUSB_GAMEPAD_Y, "START": V.XUSB_GAMEPAD_START,
     "UP": V.XUSB_GAMEPAD_DPAD_UP, "DOWN": V.XUSB_GAMEPAD_DPAD_DOWN, "LEFT": V.XUSB_GAMEPAD_DPAD_LEFT, "RIGHT": V.XUSB_GAMEPAD_DPAD_RIGHT,
     "LB": V.XUSB_GAMEPAD_LEFT_SHOULDER, "RB": V.XUSB_GAMEPAD_RIGHT_SHOULDER}
pad = vg.VX360Gamepad(); time.sleep(1.5); print("pad ready", flush=True)
while True:
    if not os.path.exists(CMD): time.sleep(0.1); continue
    time.sleep(0.05); lines = open(CMD).read().split(); os.remove(CMD)
    for s in lines:
        if s == "quit": sys.exit(0)
        if s.startswith("wait"): time.sleep(float(s[4:])); continue
        if s.startswith("ls="):
            x, y, t = map(float, s[3:].split(",")); pad.left_joystick_float(x, y); pad.update(); time.sleep(t)
            pad.left_joystick_float(0, 0); pad.update(); continue
        if s in B:
            pad.press_button(B[s]); pad.update(); time.sleep(0.35); pad.release_button(B[s]); pad.update(); time.sleep(0.3)
        print("did", s, flush=True)
