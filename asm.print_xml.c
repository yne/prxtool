
char *print_cpureg_xml(int reg, char *output){
	return output + sprintf(output, "<gpr>r%d</gpr>", reg);
}

char *print_int_xml(int i, char *output){
	return output + sprintf(output, "<imm>%d</imm>", i);
}

char *print_hex_xml(int i, char *output){
	return output + sprintf(output, "<imm>0x%X</imm>", i);
}

char *print_imm_xml(int ofs, char *output){
	if(!g_hexints)
		return output + sprintf(output, "<imm>%d</imm>", ofs);
	if((g_signedhex) && (ofs < 0))
		return output + sprintf(output, "<imm>-0x%X</imm>", -ofs);
	return output + sprintf(output, "<imm>0x%X</imm>", ofs & 0xFFFF);
}

char *print_jump_xml(unsigned int addr, char *output){
	char symbol[128];
	if(g_syms && disasmResolveRef(addr, symbol, sizeof(symbol)))
		return output + sprintf(output, "<ref>%s</ref>", symbol);
	return output + sprintf(output, "<ref>0x%08X</ref>", addr);
}

char *print_ofs_xml(int ofs, int reg, char *output){
	return print_cpureg_xml(reg, print_imm_xml(ofs, output));
}

char *print_pcofs_xml(int ofs, unsigned int PC, char *output){
	return print_jump_xml(PC + 4 + (ofs * 4), output);
}

char *print_jumpr_xml(int reg, char *output){
	return print_cpureg_xml(reg, output);
}

char *print_syscall_xml(unsigned int syscall, char *output){
	return output + sprintf(output, "<syscall>0x%X</syscall>", syscall);
}

char *print_cop0_xml(int reg, char *output){
	if(cop0_regs[reg])
		return output +  sprintf(output, "<cop0>%s</cop0>", cop0_regs[reg]);
	else
		return output +  sprintf(output, "<cop0>$%d</cop0>", reg);
}

char *print_cop1_xml(int reg, char *output){
	return output + sprintf(output, "<cop1>$fcr%d</cop1>", reg);
}

char *print_cop2_xml(int reg, char *output){
	return print_cop2(reg, output);
}

char *print_vfpu_cond_xml(int cond, char *output){
	if ((cond >= 0) && (cond < 16))
		return output + sprintf(output, "<cond>%s</cond>", vfpu_cond_names[cond]);
	else
		return output + sprintf(output, "<imm>%d</imm>", cond);
}

char *print_vfpu_const_xml(int k, char *output){
	if ((k > 0) && (k < 20))
		return output + sprintf(output, "<const>%s</const>", vfpu_const_names[k]);
	else
		return output + sprintf(output, "<imm>%d</imm>", k);
}

char *print_vfpu_halffloat_xml(int l, char *output){
	/* Convert a VFPU 16-bit floating-point number to IEEE754. */
	unsigned short float16 = l & 0xFFFF;
	unsigned int sign = (float16 >> VFPU_SH_FLOAT16_SIGN) & VFPU_MASK_FLOAT16_SIGN;
	int exponent = (float16 >> VFPU_SH_FLOAT16_EXP) & VFPU_MASK_FLOAT16_EXP;
	unsigned int fraction = float16 & VFPU_MASK_FLOAT16_FRAC;
	char signchar = '+' + ((sign == 1) * 2);

	if (exponent == VFPU_FLOAT16_EXP_MAX)
		return output + sprintf(output, fraction?"%cNaN":"%cInf", signchar);
	if (!exponent && fraction == 0)
		return output + sprintf(output, "%c0", signchar);
	if (!exponent){
		do{
			fraction <<= 1;
			exponent--;
		}while (!(fraction & (VFPU_MASK_FLOAT16_FRAC + 1)));
		fraction &= VFPU_MASK_FLOAT16_FRAC;
	}

	/* Convert to 32-bit single-precision IEEE754. */
	union float2int{unsigned int i;float f;} float2int;
	float2int.i = sign << 31;
	float2int.i |= (exponent + 112) << 23;
	float2int.i |= fraction << 13;
	return output + sprintf(output, "<float>%g</float>", float2int.f);
}

char *print_vfpu_prefix_xml(int l, unsigned int pos, char *output){
	int len = 0;

	switch (pos){
	case '0':
	case '1':
	case '2':
	case '3':{
			unsigned int base = '0';
			unsigned int negation = (l >> (pos - (base - VFPU_SH_PFX_NEG))) & VFPU_MASK_PFX_NEG;
			unsigned int constant = (l >> (pos - (base - VFPU_SH_PFX_CST))) & VFPU_MASK_PFX_CST;
			unsigned int abs_consthi = (l >> (pos - (base - VFPU_SH_PFX_ABS_CSTHI))) & VFPU_MASK_PFX_ABS_CSTHI;
			unsigned int swz_constlo = (l >> ((pos - base) * 2)) & VFPU_MASK_PFX_SWZ_CSTLO;

			len = sprintf(output, "<%s>", pfx_swz_names[pos - base]);
			if (negation)
				len = sprintf(output, "-");
			if (constant){
				len += sprintf(output, "%s", pfx_cst_names[(abs_consthi << 2) | swz_constlo]);
			}else{
				if (abs_consthi)
					len += sprintf(output, "|%s|", pfx_swz_names[swz_constlo]);
				else
					len += sprintf(output, "%s", pfx_swz_names[swz_constlo]);
			}
			len += sprintf(output, "</%s>", pfx_swz_names[pos - base]);
		}
		break;

	case '4':
	case '5':
	case '6':
	case '7':{
			unsigned int base = '4';
			unsigned int mask = (l >> (pos - (base - VFPU_SH_PFX_MASK))) & VFPU_MASK_PFX_MASK;
			unsigned int saturation = (l >> ((pos - base) * 2)) & VFPU_MASK_PFX_SAT;

			len = sprintf(output, "<%s>", pfx_swz_names[pos - base]);
			if (!mask)
				len += sprintf(output, "%s", pfx_sat_names[saturation]);
			len += sprintf(output, "</%s>", pfx_swz_names[pos - base]);
		}
		break;
	}

	return output + len;
}

char *print_vfpu_rotator_xml(int l, char *output){
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

	len = sprintf(output, "<rot>");

	for (i = 0;;){
		len += sprintf(output, "<%s>%s</%s>", pfx_swz_names[i], elements[i], pfx_swz_names[i]);
		if (++i >= opsize)
			break;
	}

	len += sprintf(output, "</rot>");

	return output + len;
}

char *print_fpureg_xml(int reg, char *output){
	int len;

	len = sprintf(output, "<fpr>$fpr%02d</fpr>", reg);

	return output + len;
}

char *print_debugreg_xml(int reg, char *output){
	int len;

	if((reg < 16) && (dr_regs[reg])){
		len = sprintf(output, "<dreg>%s</dreg>", dr_regs[reg]);
	}else{
		len = sprintf(output, "<dreg>$%02d</dreg>\n", reg);
	}

	return output + len;
}

char *print_vfpusingle_xml(int reg, char *output){
	int len;

	len = sprintf(output, "<vfpu>S%d%d%d</vfpu>", (reg >> 2) & 7, reg & 3, (reg >> 5) & 3);

	return output + len;
}

char *print_vfpu_reg_xml(int reg, int offset, char one, char two, char *output){
	int len;

	if((reg >> 5) & 1){
		len = sprintf(output, "<vfpu>%c%d%d%d</vfpu>", two, (reg >> 2) & 7, offset, reg & 3);
	}else{
		len = sprintf(output, "<vfpu>%c%d%d%d</vfpu>", one, (reg >> 2) & 7, reg & 3, offset);
	}

	return output + len;
}

char *print_vfpuquad_xml(int reg, char *output){
	return print_vfpu_reg_xml(reg, 0, 'C', 'R', output);
}

char *print_vfpupair_xml(int reg, char *output){
	if((reg >> 6) & 1){
		return print_vfpu_reg_xml(reg, 2, 'C', 'R', output);
	}else{
		return print_vfpu_reg_xml(reg, 0, 'C', 'R', output);
	}
}

char *print_vfputriple_xml(int reg, char *output){
	if((reg >> 6) & 1){
		return print_vfpu_reg_xml(reg, 1, 'C', 'R', output);
	}else{
		return print_vfpu_reg_xml(reg, 0, 'C', 'R', output);
	}
}

char *print_vfpumpair_xml(int reg, char *output){
	if((reg >> 6) & 1){
		return print_vfpu_reg_xml(reg, 2, 'M', 'E', output);
	}else{
		return print_vfpu_reg_xml(reg, 0, 'M', 'E', output);
	}
}

char *print_vfpumtriple_xml(int reg, char *output){
	if((reg >> 6) & 1){
		return print_vfpu_reg_xml(reg, 1, 'M', 'E', output);
	}else{
		return print_vfpu_reg_xml(reg, 0, 'M', 'E', output);
	}
}

char *print_vfpumatrix_xml(int reg, char *output){
	return print_vfpu_reg_xml(reg, 0, 'M', 'E', output);
}

char *print_vfpureg_xml(int reg, char type, char *output){
	switch(type){
		case 's': return print_vfpusingle_xml(reg, output);break;
		case 'q': return print_vfpuquad_xml(reg, output);break;
		case 'p': return print_vfpupair_xml(reg, output);break;
		case 't': return print_vfputriple_xml(reg, output);break;
		case 'm': return print_vfpumpair_xml(reg, output);break;
		case 'n': return print_vfpumtriple_xml(reg, output);break;
		case 'o': return print_vfpumatrix_xml(reg, output);break;
		default: break;
	};

	return output;
}

static void decode_args_xml(unsigned int opcode, unsigned int PC, const char *fmt, char *output){
	int i = 0;
	int vmmul = 0;
	int arg = 0;

	while(fmt[i]){
		if(fmt[i] == '%'){
			int len;

			i++;
			len = sprintf(output, "<arg%d>", arg);
			output += len;
			
			switch(fmt[i]){
				case 'd': output = print_cpureg_xml(RD(opcode), output);
						  break;
				case 't': output = print_cpureg_xml(RT(opcode), output);
						  break;
				case 's': output = print_cpureg_xml(RS(opcode), output);
						  break;
				case 'i': output = print_imm_xml(IMM(opcode), output);
						  break;
				case 'I': output = print_hex_xml(IMMU(opcode), output);
						  break;
				case 'o': output = print_ofs_xml(IMM(opcode), RS(opcode), output);
						  break;
				case 'O': output = print_pcofs_xml(IMM(opcode), PC, output);
						  break;
				case 'j': output = print_jump_xml(JUMP(opcode, PC), output);
						  break;
				case 'J': output = print_jumpr_xml(RS(opcode), output);
						  break;
				case 'a': output = print_int_xml(SA(opcode), output);
						  break;
				case '0': output = print_cop0_xml(RD(opcode), output);
						  break;
				case '1': output = print_cop1_xml(RD(opcode), output);
						  break;
				case 'p': *output++ = '$';
						  output = print_int_xml(RD(opcode), output);
						  break;
				case '2': // [hlide] added %2? (? is d, s)
					switch (fmt[i+1]) {
					case 'd' : output = print_cop2_xml(VED(opcode), output); i++; break;
					case 's' : output = print_cop2_xml(VES(opcode), output); i++; break;
					}
					break;
				case 'k': output = print_hex_xml(RT(opcode), output);
						  break;
				case 'D': output = print_fpureg_xml(FD(opcode), output);
						  break;
				case 'T': output = print_fpureg_xml(FT(opcode), output);
						  break;
				case 'S': output = print_fpureg_xml(FS(opcode), output);
						  break;
				case 'r': output = print_debugreg_xml(RD(opcode), output);
						  break;
				case 'n': // [hlide] completed %n? (? is e, i)
					switch (fmt[i+1]) {
					case 'e' : output = print_int_xml(RD(opcode) + 1, output); i++; break;
					case 'i' : output = print_int_xml(RD(opcode) - SA(opcode) + 1, output); i++; break;
					}
					break;
				case 'x': if(fmt[i+1]) { output = print_vfpureg_xml(VT(opcode), fmt[i+1], output); i++; }break;
				case 'y': if(fmt[i+1]) { 
							  int reg = VS(opcode);
							  if(vmmul) { if(reg & 0x20) { reg &= 0x5F; } else { reg |= 0x20; } }
							  output = print_vfpureg_xml(reg, fmt[i+1], output); i++; 
							  }
							  break;
				case 'z': if(fmt[i+1]) { output = print_vfpureg_xml(VD(opcode), fmt[i+1], output); i++; }
						  break;
				case 'v': // [hlide] completed %v? (? is 3, 5, 8, k, i, h, r, p? (? is (0, 1, 2, 3, 4, 5, 6, 7) ) )
					switch (fmt[i+1]) {
					case '3' : output = print_int_xml(VI3(opcode), output); i++; break;
					case '5' : output = print_int_xml(VI5(opcode), output); i++; break;
					case '8' : output = print_int_xml(VI8(opcode), output); i++; break;
					case 'k' : output = print_vfpu_const_xml(VI5(opcode), output); i++; break;
					case 'i' : output = print_int_xml(IMM(opcode), output); i++; break;
					case 'h' : output = print_vfpu_halffloat_xml(opcode, output); i++; break;
					case 'r' : output = print_vfpu_rotator_xml(opcode, output); i++; break;
					case 'p' : if (fmt[i+2]) { output = print_vfpu_prefix_xml(opcode, fmt[i+2], output); i += 2; }
							   break;
					}
					break;
				case 'X': if(fmt[i+1]) { output = print_vfpureg_xml(VO(opcode), fmt[i+1], output); i++; }
						  break;
				case 'Z': // [hlide] modified %Z to %Z? (? is c, n)
					switch (fmt[i+1]) {
					case 'c' : output = print_imm_xml(VCC(opcode), output); i++; break;
					case 'n' : output = print_vfpu_cond_xml(VCN(opcode), output); i++; break;
					}
					break;
				case 'c': output = print_hex_xml(CODE(opcode), output);
						  break;
				case 'C': output = print_syscall_xml(CODE(opcode), output);
						  break;
				case 'Y': output = print_ofs_xml(IMM(opcode) & ~3, RS(opcode), output);
						  break;
				case '?': vmmul = 1;
						  break;
				case 0: goto end;
				default: break;
			};
			len = sprintf(output, "</arg%d>", arg);
			output += len;
			arg++;
			i++;
		}else{
			i++;
		}
	}
end:

	*output = 0;
}

void format_line_xml(char *code, int codelen, const char *addr, unsigned int opcode, const char *name, const char *args){
	char ascii[17];
	char *p;
	int i;

	if(name == NULL){
		name = "Unknown";
		args = "";
	}

	p = ascii;
	for(i = 0; i < 4; i++){
		unsigned char ch;

		ch = (unsigned char) ((opcode >> (i*8)) & 0xFF);
		if((ch < 32) || (ch > 126)){
			ch = '.';
		}
		if(g_xmloutput && (ch == '<')){
			strcpy(p, "&lt;");
			p += strlen(p);
		}else{
			*p++ = ch;
		}
	}
	*p = 0;

	snprintf(code, codelen, "<name>%s</name><opcode>0x%08X</opcode>%s", name, opcode, args);
}

const char *disasmInstructionXML(unsigned int opcode, unsigned int PC){
	static char code[1024];
	const char *name = NULL;
	char args[1024];
	char addr[1024];
	int size;
	int i;
	Instruction *ix = NULL;

	sprintf(addr, "0x%08X", PC);
	g_regmask = 0;

	if(!g_macroon){
		size = sizeof(g_macro) / sizeof(Instruction);
		for(i = 0; i < size; i++){
			if((opcode & g_macro[i].mask) == g_macro[i].opcode){
				ix = &g_macro[i];
				break;
			}
		}
	}

	if(!ix){
		size = sizeof(g_inst) / sizeof(Instruction);
		for(i = 0; i < size; i++){
			if((opcode & g_inst[i].mask) == g_inst[i].opcode){
				ix = &g_inst[i];
				break;
			}
		}
	}

	if(ix){
		decode_args_xml(opcode, PC, ix->fmt, args);

		name = ix->name;
	}

	format_line_xml(code, sizeof(code), addr, opcode, name, args);

	return code;
}

void disasmSetXmlOutput(){
	g_xmloutput = 1;
}
