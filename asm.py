import re
import sys

value_pat = r'(?P<n>\w+)'
insts = [
    (fr'nop', 0x00),
    (fr'call {value_pat}', 0x20),
    (fr'ld a, {value_pat}', 0x10),
    (fr'swap( a, b)?', 0x30),
    (fr'ld \(b\), a', 0x60),
    (fr'ld a, \(b\)', 0x70),
    (fr'ld \({value_pat}\), a', 0x40),
    (fr'ld a, \({value_pat}\)', 0x50),
    (fr'(math )?add', 0x80),
    (fr'(math )?sub', 0x81),
    (fr'(math )?and', 0x82),
    (fr'(math )?or',  0x83),
    (fr'(math )?xor', 0x84),
    (fr'(math )?sl', 0x85),
    (fr'(math )?sr', 0x86),
    (fr'jp {value_pat}', 0x90),
    (fr'jnz {value_pat}', 0xa0),
    (fr'ret', 0xb0),
    (fr'push a', 0xc0),
    (fr'push b', 0xd0),
    (fr'pop a', 0xe0),
    (fr'pop b', 0xf0),
]
fancy_insts = [
    (fr'^$', lambda m: []),
    (fr'ld b, {value_pat}', lambda m: [0x31,0x10,m['n'],0x30]), # swap, [ld A, n], swap again
    (fr'ld b, \({value_pat}\)', lambda m: [0x31,0x50,m['n'],0x30]), # same here, but with [ld A, (n)]
    (fr'db {value_pat}', lambda m: [m['n']]),
    (fr'ld \((?P<dst>\w+)\), \((?P<src>\w+)\)', lambda m: [0xc1, 0x50, m['src'], 0x40, m['dst'], 0xe0]), # [push A], [ld A, (src)], [ld (dst), A], [pop A]
    (fr'ld \((?P<dst>\w+)\), {value_pat}', lambda m: [0xc0, 0x10, m['n'], 0x40, m['dst'], 0xe0]), # [push A], [ld A, n], [ld (dst), A], [pop A]
    (fr'push', lambda m: [0xc0, 0xd0]), # push A then B
    (fr'pop', lambda m: [0xf0, 0xe0]), # pop B then A
    (fr'jp \+(?P<n>[0-9a-f]+)', lambda m: [0x90, len(out)+int(m['n'],16)]), # jump forwards by n bytes
    (fr'jp -(?P<n>[0-9a-f]+)', lambda m: [0x90, len(out)-int(m['n'],16)]), # jump backwards by n bytes
    (fr'jnz \+(?P<n>[0-9a-f]+)', lambda m: [0xa0, len(out)+int(m['n'],16)]), # jnz forwards by n bytes
    (fr'jnz -(?P<n>[0-9a-f]+)', lambda m: [0xa0, len(out)-int(m['n'],16)]), # jnz backwards by n bytes
    # (r'halt', lambda m: [0x40, 0xfe]),
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

    #TODO: rename. this represents if we've assembled it already (to avoid accidentally assembling it twice, ex. as a normal inst *and* a fancy_inst)
    found = False

    if re.match(r'\w+:', s):
        labels[s.removesuffix(":")] = len(out)
        found = True

    to_add = []

    if not found:
        for (pat, op) in insts:
            if m := re.match(pat, s):
                d = m.groupdict()
                print(f"yeah ts is {pat}")
                to_add.append(op)
                if 'n' in d.keys():
                    to_add.append(d['n'])
                found = True
                break

    if not found:
        for (pat, fn) in fancy_insts:
            if m := re.match(pat, s):
                d = m.groupdict()
                print(f"wow ts is fancy: {pat}")
                to_add.extend(fn(d))
                found = True
                break

    if not found:
        print(f"\x1b[31m omg idk what that means: {s}") # the lack of color resetting is kinda intentional

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

with open(sys.argv[1] if len(sys.argv) == 2 else "in.s",'r') as f:
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
        exit(1)
    c_out += f"ram[0x{i:02x}]=0x{out[i]:02x};\n"

with open('out.h','w') as f:
    f.write(c_out)

lst += "\n\nlabels:\n"
for key in labels.keys():
    lst += f"    {key} = {labels[key]}\n"

with open("out.lst",'w') as f:
    f.write(lst)

# print(labels)
print("ok im done now")
