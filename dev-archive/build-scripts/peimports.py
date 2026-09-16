import struct, sys, os

def rva_to_off(sections, rva):
    for va, vsz, praw, rsz in sections:
        if va <= rva < va + max(vsz, rsz):
            return praw + (rva - va)
    return None

def imports(path):
    d = open(path, 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3C)[0]
    assert d[pe:pe+4] == b'PE\0\0', 'not a PE'
    nsec = struct.unpack_from('<H', d, pe + 6)[0]
    optsz = struct.unpack_from('<H', d, pe + 20)[0]
    opt = pe + 24
    magic = struct.unpack_from('<H', d, opt)[0]
    # data directory offset differs for PE32 vs PE32+
    ddoff = opt + (112 if magic == 0x20b else 96)
    sec = pe + 24 + optsz
    sections = []
    for i in range(nsec):
        s = sec + i * 40
        va, praw = struct.unpack_from('<I', d, s + 12)[0], struct.unpack_from('<I', d, s + 20)[0]
        vsz, rsz = struct.unpack_from('<I', d, s + 8)[0], struct.unpack_from('<I', d, s + 16)[0]
        sections.append((va, vsz, praw, rsz))

    out = {'static': [], 'delay': []}
    # 1 = import table, 13 = delay import
    for name, idx, stride, namefield in (('static', 1, 20, 12), ('delay', 13, 32, 4)):
        trva, tsz = struct.unpack_from('<II', d, ddoff + idx * 8)
        if not trva:
            continue
        off = rva_to_off(sections, trva)
        if off is None:
            continue
        i = 0
        while True:
            e = off + i * stride
            if e + stride > len(d):
                break
            chunk = d[e:e+stride]
            if chunk == b'\0' * stride:
                break
            nrva = struct.unpack_from('<I', d, e + namefield)[0]
            if not nrva:
                break
            no = rva_to_off(sections, nrva)
            if no is None:
                break
            end = d.index(b'\0', no)
            out[name].append(d[no:end].decode('ascii', 'replace'))
            i += 1
    return out

for p in sys.argv[1:]:
    print('===', os.path.basename(p), '===')
    try:
        r = imports(p)
        print('  STATIC (must be present at launch):')
        for n in sorted(set(r['static'])):
            print('    ', n)
        if r['delay']:
            print('  DELAY-LOADED (optional until used):')
            for n in sorted(set(r['delay'])):
                print('    ', n)
    except Exception as ex:
        print('  error:', ex)
