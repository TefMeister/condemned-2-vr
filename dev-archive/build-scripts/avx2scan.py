import struct, sys, collections
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

# AVX2-only mnemonics (integer ops widened to 256-bit, gathers, cross-lane permutes).
AVX2_ONLY = {
    'vpbroadcastb','vpbroadcastw','vpbroadcastd','vpbroadcastq',
    'vbroadcasti128','vinserti128','vextracti128','vperm2i128',
    'vpermd','vpermq','vpermps','vpermpd',
    'vpgatherdd','vpgatherdq','vpgatherqd','vpgatherqq',
    'vgatherdps','vgatherdpd','vgatherqps','vgatherqpd',
    'vpsllvd','vpsllvq','vpsrlvd','vpsrlvq','vpsravd',
    'vpmaskmovd','vpmaskmovq','vpblendd',
}

def sections(d):
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    optsz = struct.unpack_from('<H', d, pe + 20)[0]
    base = pe + 24 + optsz
    out = []
    for i in range(nsec):
        s = base + i * 40
        name = d[s:s+8].rstrip(b'\0').decode('ascii', 'replace')
        vsz, va, rsz, praw = struct.unpack_from('<IIII', d, s + 8)
        chars = struct.unpack_from('<I', d, s + 36)[0]
        out.append((name, va, vsz, praw, rsz, chars))
    return out

for path in sys.argv[1:]:
    d = open(path, 'rb').read()
    md = Cs(CS_ARCH_X86, CS_MODE_64)
    hits = collections.Counter()
    ymm = 0
    total = 0
    first = None
    for name, va, vsz, praw, rsz, chars in sections(d):
        if not (chars & 0x20000000):      # IMAGE_SCN_MEM_EXECUTE
            continue
        code = d[praw:praw + rsz]
        for ins in md.disasm(code, va):
            total += 1
            if 'ymm' in ins.op_str:
                ymm += 1
            if ins.mnemonic in AVX2_ONLY:
                hits[ins.mnemonic] += 1
                if first is None:
                    first = (ins.address, ins.mnemonic, ins.op_str)
    print('===', path.split('/')[-1], '===')
    print(f'  instructions decoded: {total:,}')
    print(f'  touching ymm (256-bit, needs AVX): {ymm:,}')
    print(f'  AVX2-only instructions: {sum(hits.values()):,}')
    for m, c in hits.most_common(8):
        print(f'      {m:<16} x{c}')
    if first:
        print(f'  first at RVA 0x{first[0]:x}: {first[1]} {first[2]}')
    print()
