#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define LOGd(x) printf(#x "=%d\n", x);

#define STACK_SIZE 256

typedef unsigned char u8;
typedef char s8;

int debug = 0;

struct mop_t {
	u8 mem_read : 1;
	u8 mem_write : 1;
	u8 stack_read : 1;
	u8 stack_write : 1;
	u8 update_z : 1;
	u8 addr : 2;
	u8 bus : 3;
	u8 store : 2;
	u8 jump : 2;
	u8 out : 2;
};
typedef struct mop_t mop_t;

enum mode_t {
	MODE_RESET = 0,
	MODE_LOAD,
	MODE_RUN,
	MODE_JUMP
};

struct cpu_t {
	u8 A, B, PC;
	u8 i;
	u8 o;
	u8 Z : 1;
	u8 m : 2;
	u8 j : 2;

	u8 mode : 2;

	mop_t code[3];

	u8 (*memr)(u8 addr);
	void (*memw)(u8 addr, u8 data);
	u8 (*pop)();
	void (*push)(u8 val);
};
typedef struct cpu_t cpu_t;



cpu_t the_cpu;




// thx clud
mop_t MICROCODE_ROM[16][3] = {
    // 0: nop
    {(mop_t){0}, (mop_t){0}, (mop_t){0}},
    // 1: ld A, n
    {(mop_t){.addr=1, .bus=1, .mem_read=1, .store=1, .jump=3}, (mop_t){0}, (mop_t){0}},
	// 2: call n
{(mop_t){.addr=1, .bus=1, .mem_read=1, .store=3}, (mop_t){.out=3, .stack_write=1, .jump=1}, (mop_t){0}},
    // 3: swap A, B
    {(mop_t){.bus=2, .store=3}, (mop_t){.bus=3, .store=1}, (mop_t){.bus=4, .store=2}},
    // 4: ld (n), A
    {(mop_t){.addr=1, .bus=1, .mem_read=1, .store=3}, (mop_t){.addr=2, .out=1, .mem_write=1, .jump=3}, (mop_t){0}},
    // 5: ld A, (n)
    {(mop_t){.addr=1, .bus=1, .mem_read=1, .store=3}, (mop_t){.addr=2, .bus=1, .mem_read=1, .store=1, .jump=3}, (mop_t){0}},
    // 6: ld (B), A
    {(mop_t){.addr=3, .out=1, .mem_write=1}, (mop_t){0}, (mop_t){0}},
    // 7: ld A, (B)
    {(mop_t){.addr=3, .bus=1, .mem_read=1, .store=1}, (mop_t){0}, (mop_t){0}},
    // 8: math op
    {(mop_t){.bus=5, .store=1, .update_z=1}, (mop_t){0}, (mop_t){0}},
    // 9: jp n
    {(mop_t){.addr=1, .bus=1, .mem_read=1, .store=3}, (mop_t){.jump=1}, (mop_t){0}},
    // A: jnz n
    {(mop_t){.addr=1, .bus=1, .mem_read=1, .store=3}, (mop_t){.jump=2}, (mop_t){0}},
	// B: ret
	{(mop_t){.bus=1, .stack_read=1, .store=3}, (mop_t){.jump=1}, (mop_t){0}},
    // C: push A
    {(mop_t){.out=1, .stack_write=1}, (mop_t){0}, (mop_t){0}},
    // D: push B
    {(mop_t){.out=2, .stack_write=1}, (mop_t){0}, (mop_t){0}},
    // E: pop A
    {(mop_t){.bus=1, .stack_read=1, .store=1}, (mop_t){0}, (mop_t){0}},
    // F: pop B
    {(mop_t){.bus=1, .stack_read=1, .store=2}, (mop_t){0}, (mop_t){0}},
};

void print_cpu(cpu_t c) {
	printf("PC=%02x A=%02x B=%02x i=%02x Z=%d m=%d j=%d mode=%d\n",
		c.PC, c.A, c.B, c.i, c.Z, c.m, c.j, c.mode);
	for (int i=0;i<3;i++) {
		/* printf("  code[%d]=%04x\n", i, c.code[i]); */
	}
}


void cpu_do_reset(cpu_t* c) {
	/* if (debug) printf("cpu reset\n"); */
	c->A = 0;
	c->B = 0;
	c->PC = 0;
	c->i = 0;
	c->o = 0;
	c->Z = 0;
	c->m = 0;
	c->j = 0;
	c->code[0] = (mop_t){0};
	c->code[1] = (mop_t){0};
	c->code[2] = (mop_t){0};
}
void cpu_do_load(cpu_t* c) {
	/* if (debug) printf("cpu load\n"); */
	// read at PC
	u8 opcode = c->memr(c->PC);
	u8 inst = (opcode>>4) & 0x0f; // firt nibble

	c->o = opcode;
	c->m = 0;
	c->i = 0;
	c->j = 0;

	c->code[0] = MICROCODE_ROM[inst][0];
	c->code[1] = MICROCODE_ROM[inst][1];
	c->code[2] = MICROCODE_ROM[inst][2];

	c->mode = MODE_RUN;
}

void cpu_do_run(cpu_t* c) {
	/* if (debug) printf("cpu run\n"); */
	mop_t m = c->code[c->m];

	// find addr_val
	u8 addr_val = 0;
	switch (m.addr) {
		case 0: addr_val = 0; break;
		case 1: addr_val = c->PC+1; break;
		case 2: addr_val = c->i; break;
		case 3: addr_val = c->B; break;
	}

	// find alu_val
	u8 alu_val = 0;
	switch (c->o & 0x0f) {
		case 0: alu_val = c->A + c->B; break;
		case 1: alu_val = c->A - c->B; break;
		case 2: alu_val = c->A & c->B; break;
		case 3: alu_val = c->A | c->B; break;
		case 4: alu_val = c->A ^ c->B; break;
		case 5: alu_val = c->A >> 1; break;
		case 6: alu_val = c->A << 1; break;
		case 7: alu_val = c->A; break;
		default:
			print_cpu(the_cpu);
			printf("WARNING! bad math value %d\n", c->o & 0xf0f);
	}

	// find bus_val
	u8 bus_val = 0;
	switch (m.bus) {
		case 0: bus_val = 0; break;
		case 1:
			if (m.mem_read) {
				bus_val = c->memr(addr_val);
			} else if (m.stack_read) {
				bus_val = c->pop();
			} else {
				bus_val = 0; // TODO: some invalid state or warning?
			}
			break;
		case 2: bus_val = c->A; break;
		case 3: bus_val = c->B; break;
		case 4: bus_val = c->i; break;
		case 5: bus_val = alu_val; break; //TODO: alu out
		default:
			printf("WARNING! unimplemented bus value %d\n", m.bus);
	}

	c->A = (m.store == 1) ? bus_val : c->A;
	c->B = (m.store == 2) ? bus_val : c->B;
	c->i = (m.store == 3) ? bus_val : c->i;
	c->j = m.jump;
	c->m = c->m+1;
	c->Z = m.update_z ? (alu_val == 0) : c->Z;

	u8 out_val = 0;
	switch (m.out) {
		case 0: out_val = 0; break;
		case 1: out_val = c->A; break;
		case 2: out_val = c->B; break;
		case 3: out_val = c->PC+2; break;
	}
	if (m.mem_write) {
		c->memw(addr_val, out_val);
	}
	if (m.stack_write) {
		c->push(out_val);
	}

	if (c->m == 3 || c->j != 0) {
		c->mode = MODE_JUMP;
	}
}

void cpu_do_jump(cpu_t* c) {
	/* if (debug) printf("cpu jump\n"); */
	switch (c->j) {
		case 0: c->PC = c->PC+1; break;
		case 1: c->PC = c->i; break;
		case 2: c->PC = (!c->Z) ? c->i : c->PC+2; break;
		case 3: c->PC = c->PC+2; break;
	}

	c->mode = MODE_LOAD;
}

void cpu_do_tick(cpu_t* c) {
	switch (c->mode) {
		case MODE_RESET: cpu_do_reset(c); break;
		case MODE_LOAD: cpu_do_load(c); break;
		case MODE_RUN: cpu_do_run(c); break;
		case MODE_JUMP: cpu_do_jump(c); break;
	}
}
void cpu_do_full_cycle(cpu_t* c) {
	cpu_do_tick(c);
	while (c->mode != MODE_LOAD) {
		cpu_do_tick(c);
	}
}



u8 ram[256];
u8 my_memr(u8 addr) {
	u8 val = ram[addr];
	if (addr == 0xff) {
		printf("input: ");
		scanf("%hhd",&val);
	}
	if (debug) printf("memr(%02x) -> %02x\n", addr, val);
	return val;
}
void my_memw(u8 addr, u8 val) {
	if (debug) printf("memw(%02x, %02x)\n", addr, val);
	if (addr==0xff) {
		printf("output: %d / $%02x / '%c'\n", val, val, isprint(val)?val:' ');
	} else if (addr==0xfe) {
		printf("halt\n");
		exit(0);
	}
	ram[addr]=val;
}
u8 stack[STACK_SIZE];
int stackptr = 0;
void my_push(u8 val) {
	if (debug) printf("push(%02x) stackptr=%d\n", val, stackptr);
	stack[stackptr] = val;
	stackptr++;
}
u8 my_pop() {
	if (debug) printf("pop() -> %02x stackptr=%d\n", stack[stackptr-1], stackptr-1);
	stackptr--;
	return stack[stackptr];
}

int main(int argc, char** argv) {
	
	for (int i=0;i<argc;i++) {
		if (!strcmp(argv[i],"-d")) {
			debug = 1;
		}
	}

#include "out.h"

	the_cpu.memr = my_memr;
	the_cpu.memw = my_memw;
	the_cpu.pop = my_pop;
	the_cpu.push = my_push;
	the_cpu.mode = MODE_LOAD;

	cpu_do_reset(&the_cpu);

	print_cpu(the_cpu);
	int i;
	for (i=0;i<100000;i++) {
		cpu_do_full_cycle(&the_cpu);
		if (debug) print_cpu(the_cpu);
	}
	printf("woah %d cycles? im done\n",i);

	return 0;
}
