import struct, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

path = sys.argv[1]
rva = int(sys.argv[2], 16)

d = open(path, 'rb').read()
pe = struct.unpack_from('<I', d, 0x3C)[0]
nsec = struct.unpack_from('<H', d, pe + 6)[0]
optsz = struct.unpack_from('<H', d, pe + 20)[0]
sec = pe + 24 + optsz
secs = []
for i in range(nsec):
    s = sec + i * 40
    name = d[s:s+8].rstrip(b'\0').decode('ascii', 'replace')
    vsz, va, rsz, praw = struct.unpack_from('<IIII', d, s + 8)
    secs.append((name, va, vsz, praw, rsz))

off = None
for name, va, vsz, praw, rsz in secs:
    if va <= rva < va + max(vsz, rsz):
        off = praw + (rva - va)
        print(f'RVA 0x{rva:x} is in section {name} -> file offset 0x{off:x}')
        break
if off is None:
    sys.exit('RVA not mapped')

# disassemble a window starting a little before, then mark the target
start_rva = rva - 0x40
start_off = off - 0x40
md = Cs(CS_ARCH_X86, CS_MODE_64)
md.detail = False
code = d[start_off:start_off + 0xA0]
for ins in md.disasm(code, start_rva):
    mark = '  <<<< FAULTING INSTRUCTION' if ins.address == rva else ''
    print(f'  0x{ins.address:08x}:  {ins.mnemonic:<12}{ins.op_str}{mark}')
