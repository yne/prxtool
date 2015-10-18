char* print_cpureg(int reg, char *output){
	int len;

	if(!g_mregs){
		len = sprintf(output, "$%s", regName[reg]);
	}else{
		if(reg > 0){
			len = sprintf(output, "r%d", reg);
		}else{
			*output = '0';
			*(output+1) = 0;
			len = 1;
		}
	}

	if(g_printregs){
		g_regmask |= (1 << reg);
	}

	return output + len;
}

char* print_int(int i, char *output){
	return output + sprintf(output, "%d", i);
}

char* print_hex(int i, char *output){
	return output + sprintf(output, "0x%X", i);
}

char* print_imm(int ofs, char *output){
	if(!g_hexints)
		return output + sprintf(output, "%d", ofs);
	if((g_signedhex) && (ofs < 0))
		return output + sprintf(output, "-0x%X", -ofs);
	return output + sprintf(output, "0x%X", ofs & 0xFFFF);
}

char* print_jump(unsigned int addr, char *output){
	char symbol[128];
	if(!disasmResolveSymbol(addr, symbol, sizeof(symbol)))
		return output +  sprintf(output, "0x%08X", addr);
	if(g_xmloutput)
		return output + sprintf(output, "<a href=\"#%s\">%s</a>", symbol, symbol);
	return output + sprintf(output, "%s", symbol);
}

char* print_ofs(int ofs, int reg, char *output, unsigned int *realregs){
	if((g_printreal) && (realregs))
		return output = print_jump(realregs[reg] + ofs, output);
	output = print_imm(ofs, output);
	*output++ = '(';
	output = print_cpureg(reg, output);
	*output++ = ')';

	return output;
}

char* print_pcofs(int ofs, unsigned int PC, char *output){
	return print_jump(PC + 4 + (ofs * 4), output);
}

char* print_jumpr(int reg, char *output, unsigned int *realregs){
	if((g_printreal) && (realregs))
		return print_jump(realregs[reg], output);
	return print_cpureg(reg, output);
}

char* print_syscall(unsigned int syscall, char *output){
	return output + sprintf(output, "0x%X", syscall);
}

char* print_cop0(int reg, char *output){
	if(cop0_regs[reg])
		return output + sprintf(output, "%s", cop0_regs[reg]);
	else
		return output + sprintf(output, "$%d", reg);
}

char* print_cop1(int reg, char *output){
	return output + sprintf(output, "$fcr%d", reg);
}

char* print_vfpu_prefix(int l, unsigned int pos, char *output){
	int len = 0;

	switch (pos){
	case '0':
	case '1':
	case '2':
	case '3':{
			unsigned int base = '0';
			char* sign = (l >> (pos - (base - VFPU_SH_PFX_NEG))) & VFPU_MASK_PFX_NEG ? "-":"";
			unsigned int constant    = (l >> (pos - (base - VFPU_SH_PFX_CST))) & VFPU_MASK_PFX_CST;
			unsigned int abs_consthi = (l >> (pos - (base - VFPU_SH_PFX_ABS_CSTHI))) & VFPU_MASK_PFX_ABS_CSTHI;
			unsigned int swz_constlo = (l >> ((pos - base) * 2)) & VFPU_MASK_PFX_SWZ_CSTLO;

			if (constant)
				return output + sprintf(output+len, "%s%s", sign, pfx_cst_names[(abs_consthi << 2) | swz_constlo]);
			else
				return output + sprintf(output+len, abs_consthi?"%s|%s|":"%s%s", sign, pfx_swz_names[swz_constlo]);
		}
	case '4':
	case '5':
	case '6':
	case '7':{
			unsigned int base = '4';
			if ((l >> (pos - (base - VFPU_SH_PFX_MASK))) & VFPU_MASK_PFX_MASK)
				return output + sprintf(output, "m");
			else
				return output + sprintf(output, "%s", pfx_sat_names[(l >> ((pos - base) * 2)) & VFPU_MASK_PFX_SAT]);
		}
	}

	return output;
}

char* print_vfpu_rotator(int l, char *output){
	int len;

	const char *elements[4];

	unsigned int opcode = l & VFPU_MASK_OP_SIZE;
	unsigned int rotators = (l >> 16) & 0x1f;
	unsigned int opsize, rothi, rotlo, negation, i;

	/* Determine the operand size so we'll know how many elements to output. */
	if (opcode == VFPU_OP_SIZE_PAIR)
		opsize = 2;
	else if (opcode == VFPU_OP_SIZE_TRIPLE)
		opsize = 3;
	else
		opsize = (opcode == VFPU_OP_SIZE_QUAD) * 4;	/* Sanity check. */

	rothi = (rotators >> VFPU_SH_ROT_HI) & VFPU_MASK_ROT_HI;
	rotlo = (rotators >> VFPU_SH_ROT_LO) & VFPU_MASK_ROT_LO;
	negation = (rotators >> VFPU_SH_ROT_NEG) & VFPU_MASK_ROT_NEG;

	if (rothi == rotlo){
		if (negation){
			elements[0] = "-s";
			elements[1] = "-s";
			elements[2] = "-s";
			elements[3] = "-s";
		}else{
			elements[0] = "s";
			elements[1] = "s";
			elements[2] = "s";
			elements[3] = "s";
		}
	}else{
		elements[0] = "0";
		elements[1] = "0";
		elements[2] = "0";
		elements[3] = "0";
	}
	if (negation)
		elements[rothi] = "-s";
	else
		elements[rothi] = "s";
	elements[rotlo] = "c";

	len = sprintf(output, "[");

	for (i = 0;;){
		len += sprintf(output, "%s", elements[i++]);
		if (i >= opsize)
			break;
		sprintf(output, " ,");
	}

	len += sprintf(output, "]");

	return output + len;
}

char* print_fpureg(int reg, char *output){
	int len;

	len = sprintf(output, "$fpr%02d", reg);

	return output + len;
}

char* print_debugreg(int reg, char *output){
	int len;

	if((reg < 16) && (dr_regs[reg])){
		len = sprintf(output, "%s", dr_regs[reg]);
	}else{
		len = sprintf(output, "$%02d\n", reg);
	}

	return output + len;
}

char* print_vfpusingle(int reg, char *output){
	int len;

	len = sprintf(output, "S%d%d%d", (reg >> 2) & 7, reg & 3, (reg >> 5) & 3);

	return output + len;
}

char* print_vfpu_reg(int reg, int offset, char one, char two, char *output){
	int len;

	if((reg >> 5) & 1){
		len = sprintf(output, "%c%d%d%d", two, (reg >> 2) & 7, offset, reg & 3);
	}else{
		len = sprintf(output, "%c%d%d%d", one, (reg >> 2) & 7, reg & 3, offset);
	}

	return output + len;
}

char* print_vfpuquad(int reg, char *output){
	return print_vfpu_reg(reg, 0, 'C', 'R', output);
}

char* print_vfpupair(int reg, char *output){
	return print_vfpu_reg(reg, (reg >> 6)&1?2:0, 'C', 'R', output);
}

char* print_vfputriple(int reg, char *output){
	return print_vfpu_reg(reg, (reg >> 6)&1?1:0, 'C', 'R', output);
}

char* print_vfpumpair(int reg, char *output){
	return print_vfpu_reg(reg, (reg >> 6)&1?2:0, 'M', 'E', output);
}

char* print_vfpumtriple(int reg, char *output){
	return print_vfpu_reg(reg, (reg >> 6)&1?1:0, 'M', 'E', output);
}

char* print_vfpumatrix(int reg, char *output){
	return print_vfpu_reg(reg, 0, 'M', 'E', output);
}

char* print_vfpureg(int reg, char type, char *output){
	switch(type){
		case 's': return print_vfpusingle (reg, output);break;
		case 'q': return print_vfpuquad   (reg, output);break;
		case 'p': return print_vfpupair   (reg, output);break;
		case 't': return print_vfputriple (reg, output);break;
		case 'm': return print_vfpumpair  (reg, output);break;
		case 'n': return print_vfpumtriple(reg, output);break;
		case 'o': return print_vfpumatrix (reg, output);break;
		default: break;
	};

	return output;
}

char* print_cop2(int reg, char *output){
	char* vfpu_extra_regs[] ={
		"PFXS","PFXT","PFXD","CC  ","INF4","REG5","REG6","REV",
		"RCX0","RCX1","RCX2","RCX3","RCX4","RCX5","RCX6","RCX7"
	};
	if ((reg >= 128) && (reg < 128+16) && (vfpu_extra_regs[reg - 128]))
		return output + sprintf(output, "VFPU_%s", vfpu_extra_regs[reg - 128]);
	else
		return output + sprintf(output, "$%d", reg);
}

char* print_vfpu_cond(int cond, char *output){
	if ((cond >= 0) && (cond < 16))
		return output + sprintf(output, "%s", vfpu_cond_names[cond]);
	else
		return output + sprintf(output, "%d", cond);
}

char* print_vfpu_const(int k, char *output){
	if ((k > 0) && (k < 20))
		return output + sprintf(output, "%s", vfpu_const_names[k]);
	else
		return output + sprintf(output, "%d", k);
}

char* print_vfpu_halffloat(int l, char *output){
	/* Convert a VFPU 16-bit floating-point number to IEEE754. */
	union float2int{
		unsigned int i;
		float f;
	} float2int;
	unsigned short float16 = l & 0xFFFF;
	unsigned int sign = (float16 >> VFPU_SH_FLOAT16_SIGN) & VFPU_MASK_FLOAT16_SIGN;
	int exponent = (float16 >> VFPU_SH_FLOAT16_EXP) & VFPU_MASK_FLOAT16_EXP;
	unsigned int fraction = float16 & VFPU_MASK_FLOAT16_FRAC;
	char signchar = '+' + ((sign == 1) * 2);

	if (exponent == VFPU_FLOAT16_EXP_MAX){
		if (fraction == 0)
			return output + sprintf(output, "%cInf", signchar);
		else
			return output + sprintf(output, "%cNaN", signchar);
	}
	if (exponent == 0 && fraction == 0)
		return output + sprintf(output, "%c0", signchar);
	
	if (exponent == 0){
		do{
			fraction <<= 1;
			exponent--;
		}while (!(fraction & (VFPU_MASK_FLOAT16_FRAC + 1)));

		fraction &= VFPU_MASK_FLOAT16_FRAC;
	}

	/* Convert to 32-bit single-precision IEEE754. */
	float2int.i = sign << 31;
	float2int.i |= (exponent + 112) << 23;
	float2int.i |= fraction << 13;
	return output + sprintf(output, "%g", float2int.f);
}

void decode_args(unsigned int opcode, unsigned int PC, const char *fmt, char *output, unsigned int *realregs){
	int vmmul = 0;

	for(int i = 0;fmt[i]; i++){
		if(fmt[i] != '%'){
			*output++ = fmt[i];
			continue;
		}
		i++;//skip the '%'
		switch(fmt[i]){
			case 'd': output = print_cpureg(RD(opcode), output);break;
			case 't': output = print_cpureg(RT(opcode), output);break;
			case 's': output = print_cpureg(RS(opcode), output);break;
			case 'i': output = print_imm(IMM(opcode), output);break;
			case 'I': output = print_hex(IMMU(opcode), output);break;
			case 'o': output = print_ofs(IMM(opcode), RS(opcode), output, realregs);break;
			case 'O': output = print_pcofs(IMM(opcode), PC, output);break;
			case 'j': output = print_jump(JUMP(opcode, PC), output);break;
			case 'J': output = print_jumpr(RS(opcode), output, realregs);break;
			case 'a': output = print_int(SA(opcode), output);break;
			case '0': output = print_cop0(RD(opcode), output);break;
			case '1': output = print_cop1(RD(opcode), output);break;
			case 'p': *output++ = '$';output = print_int(RD(opcode), output);break;
			case '2': // [hlide] added %2? (? is d, s)
				switch (fmt[i+1]) {
				case 'd' : output = print_cop2(VED(opcode), output); i++; break;
				case 's' : output = print_cop2(VES(opcode), output); i++; break;
				}break;
			case 'k': output = print_hex(RT(opcode), output);break;
			case 'D': output = print_fpureg(FD(opcode), output);break;
			case 'T': output = print_fpureg(FT(opcode), output);break;
			case 'S': output = print_fpureg(FS(opcode), output);break;
			case 'r': output = print_debugreg(RD(opcode), output);break;
			case 'n': // [hlide] completed %n? (? is e, i)
				switch (fmt[i+1]) {
				case 'e' : output = print_int(RD(opcode) + 1, output); i++; break;
				case 'i' : output = print_int(RD(opcode) - SA(opcode) + 1, output); i++; break;
				}break;
			case 'x': if(fmt[i+1]) { output = print_vfpureg(VT(opcode), fmt[i+1], output); i++; }break;
			case 'y': if(fmt[i+1]) {
				int reg = VS(opcode);
				if(vmmul) { if(reg & 0x20) { reg &= 0x5F; } else { reg |= 0x20; } }
				output = print_vfpureg(reg, fmt[i+1], output); i++; 
				}break;
			case 'z': if(fmt[i+1]) { output = print_vfpureg(VD(opcode), fmt[i+1], output); i++; }break;
			case 'v': // [hlide] completed %v? (? is 3, 5, 8, k, i, h, r, p? (? is (0, 1, 2, 3, 4, 5, 6, 7) ) )
				switch (fmt[i+1]) {
				case '3' : output = print_int(VI3(opcode), output); i++; break;
				case '5' : output = print_int(VI5(opcode), output); i++; break;
				case '8' : output = print_int(VI8(opcode), output); i++; break;
				case 'k' : output = print_vfpu_const(VI5(opcode), output); i++; break;
				case 'i' : output = print_int(IMM(opcode), output); i++; break;
				case 'h' : output = print_vfpu_halffloat(opcode, output); i++; break;
				case 'r' : output = print_vfpu_rotator(opcode, output); i++; break;
				case 'p' : if (fmt[i+2]) { output = print_vfpu_prefix(opcode, fmt[i+2], output); i += 2; }break;
				}break;
			case 'X': if(fmt[i+1]) { output = print_vfpureg(VO(opcode), fmt[i+1], output); i++; }break;
			case 'Z': // [hlide] modified %Z to %Z? (? is c, n)
				switch (fmt[i+1]) {
				case 'c' : output = print_imm(VCC(opcode), output); i++; break;
				case 'n' : output = print_vfpu_cond(VCN(opcode), output); i++; break;
				}break;
			case 'c': output = print_hex(CODE(opcode), output);break;
			case 'C': output = print_syscall(CODE(opcode), output);break;
			case 'Y': output = print_ofs(IMM(opcode) & ~3, RS(opcode), output, realregs);break;
			case '?': vmmul = 1;break;
			case 0: *output = fmt[i]; return;
			default: break;
		};
	}
	*output = 0;
}

int format_line(char *code, int codelen, const char *addr, unsigned int opcode, const char *name, const char *args, int noaddr){
	if(!name){
		name = "Unknown";
		args = "";
	}
	char ascii[17],*p = ascii;
	for(int i = 0; i < 4; i++){
		unsigned char ch = (unsigned char) ((opcode >> (i*8)) & 0xFF);
		if((ch < 32) || (ch > 126))
			ch = '.';
		if(g_xmloutput && (ch == '<'))
			p += strlen(strcpy(p, "&lt;"));
		else
			*p++ = ch;
	}
	*p = 0;

	if(noaddr)
		return snprintf(code, codelen, "%-10s %s", name, args);
	if(!g_printswap)
		return snprintf(code, codelen, "%s: 0x%08X '%s' - %-10s %s", addr, opcode, ascii, name, args);
	if(g_xmloutput)
		return snprintf(code, codelen, "%-10s %-80s ; %s: 0x%08X '%s'", name, args, addr, opcode, ascii);
	else
		return snprintf(code, codelen, "%-10s %-40s ; %s: 0x%08X '%s'", name, args, addr, opcode, ascii);

}

char* disasmInstruction(unsigned int opcode, unsigned int PC, unsigned int *realregs, unsigned int *regmask, int noaddr, Instruction*macro, size_t macro_count, Instruction*inst, size_t inst_count){
	static char code[1024];
	char args[1024];
	char addr[1024];
	Instruction *ix = NULL;

	sprintf(addr, "0x%08X", PC);
	if(/*(g_syms) && (g_symaddr)*/0){
		char addrtemp[128];
		/* Symbol resolver shouldn't touch addr unless it finds symbol */
		if(disasmResolveSymbol(PC, addrtemp, sizeof(addrtemp)))
			snprintf(addr, sizeof(addr), "%-20s", addrtemp);
	}

	g_regmask = 0;
	if(!g_macroon)
		for(int i = 0; (i < macro_count) && !ix; i++)
			if((opcode & macro[i].mask) == macro[i].opcode)
				ix = &macro[i];

	for(int i = 0; (i < inst_count) && !ix; i++)
		if((opcode & inst[i].mask) == inst[i].opcode)
			ix = &inst[i];

	if(ix){
		decode_args(opcode, PC, ix->fmt, args, realregs);
		if(regmask)
			*regmask = g_regmask;
	}

	format_line(code, sizeof(code), addr, opcode, ix?ix->name:NULL, args, noaddr);

	return code;
}
