import re

insts = [
    (r'nop', 0x00),
    (r'ld a, b', 0x20),
    (r'ld a, (?P<n>\w+)', 0x10),
    (r'swap( a, b)?', 0x30),
    (r'ld \(b\), a', 0x60),
    (r'ld a, \(b\)', 0x70),
    (r'ld \((?P<n>\w+)\), a', 0x40),
    (r'ld a, \((?P<n>\w+)\)', 0x50),
    (r'(math )?add', 0x80),
    (r'(math )?sub', 0x81),
    (r'(math )?and', 0x82),
    (r'(math )?or',  0x83),
    (r'(math )?xor', 0x84),
    (r'jp (?P<n>\w+)', 0x90),
    (r'jnz (?P<n>\w+)', 0xa0),
    (r'push a', 0xc0),
    (r'push b', 0xd0),
    (r'pop a', 0xe0),
    (r'pop b', 0xf0),
]
fancy_insts = [
    (r'^$', lambda m: []),
    (r'ld b, (?P<n>\w+)', lambda m: [0x30,0x10,m['n'],0x30]), # swap, [ld A, n], swap again
    (r'ld b, \((?P<n>\w+)\)', lambda m: [0x30,0x50,m['n'],0x30]), # same here, but with [ld A, (n)]
    (r'db (?P<n>\w+)', lambda m: [m['n']]),
    (r'ld \((?P<dst>\w+)\), \((?P<src>\w+)\)', lambda m: [0xc0, 0x50, m['src'], 0x40, m['dst'], 0xe0]), # [push A], [ld A, (src)], [ld (dst), A], [pop A]
    (r'push', lambda m: [0xc0, 0xd0]),
    (r'pop', lambda m: [0xf0, 0xe0]),
]

out = []

lst = ""

labels = {}

def asm_line(s:str):
    global lst
    orig = s.removesuffix("\n")
    if ';' in s:
        s = s.split(';')[0]
    s = s.lower().strip()

    if re.match(r'\w+:', s):
        labels[s.removesuffix(":")] = len(out)
        return

    to_add = []

    found = False
    for (pat, op) in insts:
        if m := re.match(pat, s):
            d = m.groupdict()
            print(f"yeah ts is {pat}")
            to_add.append(op)
            if 'n' in d.keys():
                to_add.append(d['n'])
            found = True
            break

    for (pat, fn) in fancy_insts:
        if m := re.match(pat, s):
            d = m.groupdict()
            print(f"wow ts is fancy: {pat}")
            to_add.extend(fn(d))
            found = True
            break

    if not found:
        print(f"omg idk what that means: {s}")

    lst += f"PC={len(out):02x} | "
    for i in range(4):
        if i<len(to_add):
            if type(to_add[i]) == int:
                lst += f"{to_add[i]:02x} "
            else:
                lst += f"{to_add[i]:2} "
        else:
            lst += "   "
    if i<len(to_add):
        lst += "..."
    lst += f"\t| {orig}\n"

    out.extend(to_add)

with open("in.s",'r') as f:
    for line in f:
        asm_line(line)

c_out = ""
for i in range(len(out)):
    if out[i] in labels.keys():
        out[i] = labels[out[i]]
    try:
        if type(out[i]) == str:
            out[i] = int(out[i], 16)
    except ValueError:
        print(f"damnnn {out[i]=}")
    c_out += f"ram[0x{i:02x}]=0x{out[i]:02x};\n"

with open('out.h','w') as f:
    f.write(c_out)

lst += "\n\nlabels:\n"
for key in labels.keys():
    lst += f"    {key} = {labels[key]}\n"

with open("out.lst",'w') as f:
    f.write(lst)

# print(labels)
