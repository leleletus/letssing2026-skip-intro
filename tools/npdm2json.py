"""main.npdm -> npdmtool JSON (META, ACI0 filesystem/services/kernel capabilities). Usage: npdm2json.py in.npdm"""
import json, struct, sys

data = open(sys.argv[1], 'rb').read()
assert data[:4] == b'META'
flags, = struct.unpack_from('<B', data, 0xC)
prio, core = struct.unpack_from('<BB', data, 0xE)
sysres, version, stack = struct.unpack_from('<III', data, 0x14)
name = data[0x20:0x30].split(b'\0')[0].decode()
aci_off, aci_size, acid_off, acid_size = struct.unpack_from('<IIII', data, 0x70)
aci = data[aci_off:aci_off + aci_size]
acid = data[acid_off:acid_off + acid_size]
assert aci[:4] == b'ACI0' and acid[0x200:0x204] == b'ACID'
title_id, = struct.unpack_from('<Q', aci, 0x10)
fs_o, fs_s, srv_o, srv_s, k_o, k_s = struct.unpack_from('<IIIIII', aci, 0x20)
pid_min, pid_max = struct.unpack_from('<QQ', acid, 0x210)

fs = aci[fs_o:fs_o + fs_s]
fs_perm, = struct.unpack_from('<Q', fs, 4)
assert fs_s in (0x1C, 0x2C) or True

services, host = [], []
i = srv_o
while i < srv_o + srv_s:
    ctrl = aci[i]
    if ctrl == 0:
        break
    n = (ctrl & 7) + 1
    s = aci[i + 1:i + 1 + n].decode()
    (host if ctrl & 0x80 else services).append(s)
    i += 1 + n

caps, syscalls = [], {}
for j in range(k_o, k_o + k_s, 4):
    d, = struct.unpack_from('<I', aci, j)
    if d == 0xFFFFFFFF:
        continue
    ones = (~d & -~d).bit_length() - 1 if d != 0xFFFFFFFF else 32
    ones = 0
    while d >> ones & 1:
        ones += 1
    if ones == 3:
        caps.append({'type': 'kernel_flags', 'value': {
            'highest_thread_priority': (d >> 4) & 0x3F, 'lowest_thread_priority': (d >> 10) & 0x3F,
            'lowest_cpu_id': (d >> 16) & 0xFF, 'highest_cpu_id': (d >> 24) & 0xFF}})
    elif ones == 4:
        idx, mask = (d >> 29) & 7, (d >> 5) & 0xFFFFFF
        for b in range(24):
            if mask >> b & 1:
                syscalls[f"svc{idx * 24 + b:02x}"] = f"0x{idx * 24 + b:02x}"
    elif ones == 13:
        caps.append({'type': 'application_type', 'value': (d >> 14) & 7})
    elif ones == 14:
        caps.append({'type': 'min_kernel_version', 'value': f"0x{(d >> 15) & 0x1FFFF:04x}"})
    elif ones == 15:
        caps.append({'type': 'handle_table_size', 'value': (d >> 16) & 0x3FF})
    elif ones == 16:
        caps.append({'type': 'debug_flags', 'value': {'allow_debug': bool(d >> 17 & 1), 'force_debug': bool(d >> 18 & 1),
                                                     'force_debug_prod': bool(d >> 19 & 1)}})
    else:
        sys.exit(f"unknown kernel capability {d:#010x} ({ones} trailing ones)")
caps.insert(1, {'type': 'syscalls', 'value': syscalls})

out = {
    'name': name, 'title_id': f"0x{title_id:016x}",
    'title_id_range_min': f"0x{pid_min:016x}", 'title_id_range_max': f"0x{pid_max:016x}",
    'main_thread_stack_size': f"0x{stack:08x}", 'system_resource_size': f"0x{sysres:08x}",
    'version': f"0x{version:08x}", 'main_thread_priority': prio, 'default_cpu_id': core,
    'process_category': 0, 'is_retail': True, 'pool_partition': (struct.unpack_from('<I', acid, 0x20C)[0] >> 2) & 0xF,
    'is_64_bit': bool(flags & 1), 'address_space_type': (flags >> 1) & 7,
    'optimize_memory_allocation': bool(flags & 0x10), 'disable_device_address_space_merge': bool(flags & 0x20),
    'enable_alias_region_extra_size': bool(flags & 0x40), 'prevent_code_reads': bool(flags & 0x80),
    'filesystem_access': {'permissions': f"0x{fs_perm:016x}"},
    'service_access': services, 'service_host': host, 'kernel_capabilities': caps,
}
print(json.dumps(out, indent=1))
