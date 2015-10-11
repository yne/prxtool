/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k6
 *
 * disasm.C - Implementation of a MIPS disassembler
 ***************************************************************/

#include <stdio.h>
#include <string.h>

/* Format codes
 * %d - Rd
 * %t - Rt
 * %s - Rs
 * %i - 16bit signed immediate
 * %I - 16bit unsigned immediate (always printed in hex)
 * %o - 16bit signed offset (rs base)
 * %O - 16bit signed offset (PC relative)
 * %j - 26bit absolute offset
 * %J - Register jump
 * %a - SA
 * %0 - Cop0 register
 * %1 - Cop1 register
 * %2? - Cop2 register (? is (s, d))
 * %p - General cop (i.e. numbered) register
 * %n? - ins/ext size, ? (e, i)
 * %r - Debug register
 * %k - Cache function
 * %D - Fd
 * %T - Ft
 * %S - Fs
 * %x? - Vt (? is (s/scalar, p/pair, t/triple, q/quad, m/matrix pair, n/matrix triple, o/matrix quad)
 * %y? - Vs
 * %z? - Vd
 * %X? - Vo (? is (s, q))
 * %Y - VFPU offset
 * %Z? - VFPU condition code/name (? is (c, n))
 * %v? - VFPU immediate, ? (3, 5, 8, k, i, h, r, p? (? is (0, 1, 2, 3, 4, 5, 6, 7)))
 * %c - code (for break)
 * %C - code (for syscall)
 * %? - Indicates vmmul special exception
 */

#define DISASM_OPT_MAX       8
#define DISASM_OPT_HEXINTS   'x'
#define DISASM_OPT_MREGS     'r'
#define DISASM_OPT_SYMADDR   's'
#define DISASM_OPT_MACRO     'm'
#define DISASM_OPT_PRINTREAL 'p'
#define DISASM_OPT_PRINTREGS 'g'
#define DISASM_OPT_PRINTSWAP 'w'
#define DISASM_OPT_SIGNEDHEX 'd'

#define RT(op) ((op >> 16) & 0x1F)
#define RS(op) ((op >> 21) & 0x1F)
#define RD(op) ((op >> 11) & 0x1F)
#define FT(op) ((op >> 16) & 0x1F)
#define FS(op) ((op >> 11) & 0x1F)
#define FD(op) ((op >> 6) & 0x1F)
#define SA(op) ((op >> 6)  & 0x1F)
#define IMM(op) ((signed short) (op & 0xFFFF))
#define IMMU(op) ((unsigned short) (op & 0xFFFF))
#define JUMP(op, pc) ((pc & 0xF0000000) | ((op & 0x3FFFFFF) << 2))
#define CODE(op) ((op >> 6) & 0xFFFFF)
#define SIZE(op) ((op >> 11) & 0x1F)
#define POS(op)  ((op >> 6) & 0x1F)
#define VO(op)   (((op & 3) << 5) | ((op >> 16) & 0x1F))
#define VCC(op)  ((op >> 18) & 7)
#define VD(op)   (op & 0x7F)
#define VS(op)   ((op >> 8) & 0x7F)
#define VT(op)   ((op >> 16) & 0x7F)

// [hlide] new #defines
#define VED(op)  (op & 0xFF)
#define VES(op)  ((op >> 8) & 0xFF)
#define VCN(op)  (op & 0x0F)
#define VI3(op)  ((op >> 16) & 0x07)
#define VI5(op)  ((op >> 16) & 0x1F)
#define VI8(op)  ((op >> 16) & 0xFF)

#define INSTR_TYPE_PSP    1
#define INSTR_TYPE_B      2
#define INSTR_TYPE_JUMP   4
#define INSTR_TYPE_JAL    8

#define INSTR_TYPE_BRANCH (INSTR_TYPE_B | INSTR_TYPE_JUMP | INSTR_TYPE_JAL)

#define ADDR_TYPE_NONE 0
#define ADDR_TYPE_16   1
#define ADDR_TYPE_26   2
#define ADDR_TYPE_REG  3

/* VFPU 16-bit floating-point format. */
#define VFPU_FLOAT16_EXP_MAX	0x1f
#define VFPU_SH_FLOAT16_SIGN	15
#define VFPU_MASK_FLOAT16_SIGN	0x1
#define VFPU_SH_FLOAT16_EXP	10
#define VFPU_MASK_FLOAT16_EXP	0x1f
#define VFPU_SH_FLOAT16_FRAC	0
#define VFPU_MASK_FLOAT16_FRAC	0x3ff

/* VFPU prefix instruction operands.  The *_SH_* values really specify where
   the bitfield begins, as VFPU prefix instructions have four operands
   encoded within the immediate field. */
#define VFPU_SH_PFX_NEG		16
#define VFPU_MASK_PFX_NEG	0x1	/* Negation. */
#define VFPU_SH_PFX_CST		12
#define VFPU_MASK_PFX_CST	0x1	/* Constant. */
#define VFPU_SH_PFX_ABS_CSTHI	8
#define VFPU_MASK_PFX_ABS_CSTHI	0x1	/* Abs/Constant (bit 2). */
#define VFPU_SH_PFX_SWZ_CSTLO	0
#define VFPU_MASK_PFX_SWZ_CSTLO	0x3	/* Swizzle/Constant (bits 0-1). */
#define VFPU_SH_PFX_MASK	8
#define VFPU_MASK_PFX_MASK	0x1	/* Mask. */
#define VFPU_SH_PFX_SAT		0
#define VFPU_MASK_PFX_SAT	0x3	/* Saturation. */


/* Special handling of the vrot instructions. */
#define VFPU_MASK_OP_SIZE	0x8080	/* Masks the operand size (pair, triple, quad). */
#define VFPU_OP_SIZE_PAIR	0x80
#define VFPU_OP_SIZE_TRIPLE	0x8000
#define VFPU_OP_SIZE_QUAD	0x8080
/* Note that these are within the rotators field, and not the full opcode. */
#define VFPU_SH_ROT_HI		2
#define VFPU_MASK_ROT_HI	0x3
#define VFPU_SH_ROT_LO		0
#define VFPU_MASK_ROT_LO	0x3
#define VFPU_SH_ROT_NEG		4	/* Negation. */
#define VFPU_MASK_ROT_NEG	0x1

typedef enum{
	SYMBOL_NOSYM = 0,
	SYMBOL_UNK,
	SYMBOL_FUNC,
	SYMBOL_LOCAL,
	SYMBOL_DATA,
}SymbolType;

typedef struct{
	uint32_t addr;
	SymbolType type;
	size_t size;
	char name[32];
	uint32_t*refs;
	char* alias;
	PspEntries * exported,* imported;
}SymbolEntry;

typedef struct{
	uint32_t addr;
	uint32_t target;
	int text;// Does it point to a text section ?
}ImmEntry;

typedef struct{
	size_t length;
	SymbolEntry*symbols;
}Symbols;

typedef struct{
	size_t length;
	ImmEntry *imms;
}Imms;

typedef struct{
	char *name;
	uint32_t opcode;
	uint32_t mask;
	char *fmt;
	int addrtype;
	int type;
}Instruction;

typedef struct{
	char opt;int *value;
	const char *name;
}DisasmOpt;

char* regName[32] ={
    "zr", "at", "v0", "v1", "a0", "a1", "a2", "a3",
    "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", 
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
    "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

char* cop0_regs[32] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	"BadVaddr", "Count", NULL, "Compare", "Status", "Cause", "EPC", "PrID",
	"Config", NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, "EBase", NULL, NULL, "TagLo", "TagHi", "ErrorPC", NULL
};

char* dr_regs[16] = {
	"DRCNTL", "DEPC", "DDATA0", "DDATA1", "IBC", "DBC", NULL, NULL, 
	"IBA", "IBAM", NULL, NULL, "DBA", "DBAM", "DBD", "DBDM"
};

char* vfpu_extra_regs[] ={
	"VFPU_PFXS",
	"VFPU_PFXT",
	"VFPU_PFXD",
	"VFPU_CC ",
	"VFPU_INF4",
	NULL,
	NULL,
	"VFPU_REV",
	"VFPU_RCX0",
	"VFPU_RCX1",
	"VFPU_RCX2",
	"VFPU_RCX3",
	"VFPU_RCX4",
	"VFPU_RCX5",
	"VFPU_RCX6",
	"VFPU_RCX7"
};

char* vfpu_cond_names[16] ={
  "FL",  "EQ",  "LT",  "LE",
  "TR",  "NE",  "GE",  "GT",
  "EZ",  "EN",  "EI",  "ES",
  "NZ",  "NN",  "NI",  "NS"
};

char* vfpu_const_names[20] ={
  "",
  "VFPU_HUGE",
  "VFPU_SQRT2",
  "VFPU_SQRT1_2",
  "VFPU_2_SQRTPI",
  "VFPU_2_PI",
  "VFPU_1_PI",
  "VFPU_PI_4",
  "VFPU_PI_2",
  "VFPU_PI",
  "VFPU_E",
  "VFPU_LOG2E",
  "VFPU_LOG10E",
  "VFPU_LN2",
  "VFPU_LN10",
  "VFPU_2PI",
  "VFPU_PI_6",
  "VFPU_LOG10TWO",
  "VFPU_LOG2TEN",
  "VFPU_SQRT3_2"
};

char* pfx_cst_names[8] ={
  "0",  "1",  "2",  "1/2",  "3",  "1/3",  "1/4",  "1/6"
};

char* pfx_swz_names[4] ={
  "x",  "y",  "z",  "w"
};

char* pfx_sat_names[4] ={
  "",  "[0:1]",  "",  "[-1:1]"
};

/* TODO: Add a register state block so we can convert lui/addiu to li */

int g_hexints   = 0;
int g_mregs     = 0;
int g_symaddr   = 0;
int g_macroon   = 0;
int g_printreal = 0;
int g_printregs = 0;
int g_regmask   = 0;
int g_printswap = 0;
int g_signedhex = 0;
int g_xmloutput = 0;

Symbols *g_syms = NULL;

DisasmOpt arg_disasmopts[DISASM_OPT_MAX] = {
	{ DISASM_OPT_HEXINTS  , &g_hexints  , "Hex Integers" },
	{ DISASM_OPT_MREGS    , &g_mregs    , "Mnemonic Registers" },
	{ DISASM_OPT_SYMADDR  , &g_symaddr  , "Symbol Address" },
	{ DISASM_OPT_MACRO    , &g_macroon  , "Macros" },
	{ DISASM_OPT_PRINTREAL, &g_printreal, "Print Real Address" },
	{ DISASM_OPT_PRINTREGS, &g_printregs, "Print Regs" },
	{ DISASM_OPT_PRINTSWAP, &g_printswap, "Print Swap" },
	{ DISASM_OPT_SIGNEDHEX, &g_signedhex, "Signed Hex" },
};

SymbolType disasmResolveSymbol(unsigned int PC, char *name, int namelen){
	if(g_syms && g_syms[PC].symbols)
		return snprintf(name, namelen, "%s", g_syms[PC].symbols->name),g_syms[PC].symbols->type;
	return SYMBOL_NOSYM;
}

SymbolType disasmResolveRef(unsigned int PC, char *name, int namelen){
	if(!g_syms)
		return SYMBOL_NOSYM;
	SymbolEntry *s = g_syms[PC].symbols;
	if((!s) || (!s->imported))
		return SYMBOL_NOSYM;
	unsigned int nid = 0;
	PspEntries *pImp = &s->imported[0];

	for(int i = 0; i < pImp->f_count; i++){
		if(strcmp(s->name, pImp->funcs[i].name) == 0){
			nid = pImp->funcs[i].nid;
			break;
		}
	}
	snprintf(name, namelen, "/%s/%s/nid:0x%08X", pImp->file, pImp->name, nid);
	return s->type;
}

SymbolEntry* disasmFindSymbol(unsigned int PC){
	return g_syms ? g_syms[PC].symbols : NULL;
}

int disasmIsBranch(unsigned int opcode, unsigned int PC, unsigned int *dwTarget, Instruction*inst, size_t inst_count){
	int type = 0;

	for(int i = 0; i < inst_count; i++){
		if(((opcode & inst[i].mask) == inst[i].opcode) && (inst[i].type & INSTR_TYPE_BRANCH)){
			if(inst[i].addrtype == ADDR_TYPE_16){
				if(dwTarget)*dwTarget = PC + 4 + ((signed short) (opcode & 0xFFFF)) * 4;
			}else if(inst[i].addrtype == ADDR_TYPE_26){
				if(dwTarget)*dwTarget = ((opcode & 0x03FFFFFF) << 2) + (PC & 0xF0000000);
			}else
				break;
			type = inst[i].type;
		}
	}

	return type;
}

void disasmAddBranchSymbols(unsigned int opcode, unsigned int PC, Symbols*syms, Instruction*inst, size_t inst_count){
	SymbolType type;
	unsigned int addr;
	char buf[128];

	int insttype = disasmIsBranch(opcode, PC, &addr, inst, inst_count);
	if(insttype != 0){
		if(insttype & (INSTR_TYPE_B | INSTR_TYPE_JUMP)){
			snprintf(buf, sizeof(buf), "loc_%08X", addr);
			type = SYMBOL_LOCAL;
		}else{
			snprintf(buf, sizeof(buf), "sub_%08X", addr);
			type = SYMBOL_FUNC;
		}

		SymbolEntry *s = syms[addr].symbols;
		if(!s){
			//s = SymbolEntry;
			s->addr = addr;
			s->type = type;
			s->size = 0;
			//s->name = buf;
			//s->refs.insert(s->refs.end(), PC);
			//syms[addr] = s;
		}else{
			if((s->type != SYMBOL_FUNC) && (type == SYMBOL_FUNC)){
				s->type = SYMBOL_FUNC;
			}
			//s->refs.insert(s->refs.end(), PC);
		}
	}
}

void disasmSetHexInts(int hexints){
	g_hexints = hexints;
}

void disasmSetMRegs(int mregs){
	g_mregs = mregs;
}

void disasmSetSymAddr(int symaddr){
	g_symaddr = symaddr;
}

void disasmSetMacro(int macro){
	g_macroon = macro;
}

void disasmSetPrintReal(int printreal){
	g_printreal = printreal;
}

void disasmSetSymbols(Symbols *syms){
	g_syms = syms;
}

void disasmSetOpts(const char *opts, int set){
	while(*opts){
		char ch = *opts++;
		int i;
		for(i = 0; i < DISASM_OPT_MAX; i++){
			if(ch == arg_disasmopts[i].opt){
				*arg_disasmopts[i].value = set;
				break;
			}
		}
		if(i == DISASM_OPT_MAX){
			printf("Unknown disassembler option '%c'\n", ch);
		}
	}
}

void disasmPrintOpts(void){
	int i;

	printf("Disassembler Options:\n");
	for(i = 0; i < DISASM_OPT_MAX; i++){
		printf("%c : %-3s - %s \n", arg_disasmopts[i].opt, *arg_disasmopts[i].value ? "on" : "off", 
				arg_disasmopts[i].name);
	}
}

#include "asm.print.c"
#include "asm.print_xml.c"