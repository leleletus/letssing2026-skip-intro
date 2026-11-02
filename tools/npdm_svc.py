"""Lists / adds allowed system calls in a Switch main.npdm (ACI0 and ACID kernel capabilities).
Usage: npdm_svc.py in.npdm [out.npdm 0x74,0x75,...]"""
import struct, sys

def caps_region(data, base, is_acid):
    off = 0x230 if is_acid else 0x30
    o, s = struct.unpack_from('<II', data, base + off)
    return base + o, s

def syscalls(data, start, size):
    found = {}
    for i in range(0, size, 4):
        d = struct.unpack_from('<I', data, start + i)[0]
        if d & 0x1F == 0x0F:                       # 4 trailing ones: EnableSystemCalls
            index = (d >> 29) & 7
            mask = (d >> 5) & 0xFFFFFF
            found[start + i] = (index, mask)
    return found

data = bytearray(open(sys.argv[1], 'rb').read())
assert data[:4] == b'META'
aci_off, aci_size, acid_off, acid_size = struct.unpack_from('<IIII', data, 0x70)
regions = {'ACI0': caps_region(data, aci_off, False), 'ACID': caps_region(data, acid_off, True)}
want = [int(x, 16) for x in sys.argv[3].split(',')] if len(sys.argv) > 3 else []
for name, (start, size) in regions.items():
    sc = syscalls(data, start, size)
    allowed = sorted(idx * 24 + b for idx, mask in sc.values() for b in range(24) if mask >> b & 1)
    missing = [w for w in want if w not in allowed]
    print(f"{name}: {len(allowed)} syscalls allowed; missing of the wanted: {[hex(m) for m in missing]}")
    for w in missing:
        idx, bit = divmod(w, 24)
        pos = next((p for p, (i, m) in sc.items() if i == idx), None)
        if pos is None:
            sys.exit(f"{name} has no descriptor for syscalls {idx * 24:#x}-{idx * 24 + 23:#x}: can't add {w:#x} in place")
        d = struct.unpack_from('<I', data, pos)[0] | (1 << (5 + bit))
        struct.pack_into('<I', data, pos, d)
        sc[pos] = (idx, sc[pos][1] | (1 << bit))
if len(sys.argv) > 2:
    open(sys.argv[2], 'wb').write(data)
    print('written', sys.argv[2])
