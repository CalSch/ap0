# ap0

ap0 is the latest and greatest cpu.
i actually thought about the electrical/logic design this time and *in theory* i could make this a real thing.
the problem is i dont know nothin about electrical design or anything like that.

## how does it work

it has many features that other cpu's dont, such as:
- 256 bytes of ram
- a stack (not stored in ram tho)
- 2 (two) registers, A and B
- math (add, subtract, and, or, xor, shift left, shift right)
- one (1) conditional jump instruction
- subroutines (call and return)
- no carry flag, good luck doing math

## the lore

it's called the "ap0" because i came up with its design during my leftover time from the AP CSA exam.

## usage

please don't. you deserve better.

if you really want to, it works like this:
- write your assembly in `in.s`
  - need assembly documentation? haha good luck. try reading `asm.py` and deciphering my code
- run `asm.py`
  - it will create `out.h`, which is `#include`'ed in `emu.c` and contains the C code to set the ram values. isn't it great? (um i just realized it should be `out.c` bc its code not a header, but i dont car)
- do `gcc -o ap0 emu.c`
- run `ap0`
- yuo win!
