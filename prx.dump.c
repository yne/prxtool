
void PrxPrintRow(CProcessPrx* prx,FILE *fp, const uint32_t* row, int32_t row_size, uint32_t addr){
	char buffer[512],*p = buffer;
	sprintf(p, "0x%08X - ", addr);
	p += strlen(p);

	for(int i = 0; i < 16; i++){
		if(i < row_size){
			sprintf(p, "%02X ", row[i]);
		}else{
			sprintf(p, "-- ");
		}

		p += strlen(p);

		if((i < 15) && ((i & 3) == 3)){
			*p++ = '|';
			*p++ = ' ';
		}
	}

	sprintf(p, "- ");
	p += strlen(p);

	for(int i = 0; i < 16; i++){
		if(i < row_size){
			if((row[i] >= 32) && (row[i] < 127)){
				if(prx->blXmlDump && (row[i] == '<')){
					strcpy(p, "&lt;");
					p += strlen(p);
				}else{
					*p++ = row[i];
				}
			}else{
				*p++ =  '.';
			}
		}else{
			*p++ = '.';
		}
	}
	*p = 0;
	fprintf(fp, "%s\n", buffer);
}

void PrxDumpData(CProcessPrx* prx,FILE *fp, uint32_t dwAddr, uint32_t iSize, unsigned char *pData){
	fprintf(fp, "           - 00 01 02 03 | 04 05 06 07 | 08 09 0A 0B | 0C 0D 0E 0F - 0123456789ABCDEF\n");
	fprintf(fp, "-------------------------------------------------------------------------------------\n");
	uint32_t row[16];
	memset(row, 0, sizeof(row));
	int row_size = 0;
	for(int i = 0; i < iSize; i++){
		row[row_size] = pData[i];
		row_size++;
		if(row_size == 16){
			if(prx->blXmlDump){
				fprintf(fp, "<a name=\"0x%08X\"></a>", dwAddr & ~15);
			}
			PrxPrintRow(prx, fp, row, row_size, dwAddr);
			dwAddr += 16;
			row_size = 0;
			memset(row, 0, sizeof(row));
		}
	}
	if(row_size > 0){
		if(prx->blXmlDump){
			fprintf(fp, "<a name=\"0x%08X\"></a>", dwAddr & ~15);
		}
		PrxPrintRow(prx, fp, row, row_size, dwAddr);
	}
}

int PrxReadString(CProcessPrx* prx,uint32_t dwAddr, char*str, int unicode, uint32_t *dwRet){
	int iSize = 0;//VmemGetSize(dwAddr);
	int iRealLen = 0;

	if(unicode){
		if(dwAddr & 1)// misaligned unicode word => little chance it is a valid
			return 0;
		iSize /= 2;
	}
	strcpy(str,unicode?"L\"":"\"");

	for(int i = 0; i < iSize; i++){
		// Dirty unicode, we dont _really_ care about it being unicode
		// as opposed to being 16bits 
		
		unsigned int ch = unicode?VmemGetU16(&prx->vMem,dwAddr):VmemGetU8(&prx->vMem,dwAddr);
		dwAddr+=unicode?2:1;
		if((ch == 0) && (iRealLen >= MINIMUM_STRING)){
			if(dwRet)
				*dwRet = dwAddr;
			strcpy(str,"\"");
			return 1;
		}
		if(ISSPACE(ch) || ((ch >= 32) && (ch < 127))){
			if((ch >= 32) && (ch < 127)){
				if((prx->blXmlDump) && (ch == '<'))
					strcpy(str,"&lt;");
				else
					strncpy(str,(const char*)&ch,unicode?2:1);
				iRealLen++;
			}else{
				if(ch=='\t')strcpy(str,"\\t"),iRealLen++;
				if(ch=='\r')strcpy(str,"\\r"),iRealLen++;
				if(ch=='\n')strcpy(str,"\\n"),iRealLen++;
				if(ch=='\v')strcpy(str,"\\v"),iRealLen++;
				if(ch=='\f')strcpy(str,"\\f"),iRealLen++;
			}
		}
	}
	return 0;
}

void PrxDumpStrings(CProcessPrx*prx,FILE *fp, uint32_t dwAddr, uint32_t iSize, unsigned char *pData){
	if(iSize <= MINIMUM_STRING)return;
	char* curr = "";
	int head_printed = 0;
	
	for(uint32_t dwEnd = dwAddr + iSize - MINIMUM_STRING;dwAddr < dwEnd;){
		uint32_t dwNext;
		if(PrxReadString(prx,dwAddr - prx->dwBase, curr, 0, &dwNext) || PrxReadString(prx,dwAddr - prx->dwBase, curr, 1, &dwNext)){
			if(!head_printed){
				fprintf(fp, "\n; Strings\n");
				head_printed = 1;
			}
			fprintf(fp, "0x%08X: %s\n", dwAddr, curr);
			dwAddr = dwNext + prx->dwBase;
		}else{
			dwAddr++;
		}
	}
}

void PrxDump(CProcessPrx*prx,FILE *fp, const char *disopts){
//	disasmSetSymbols(&prx->syms);
	disasmSetOpts(disopts, 1);

	if(prx->blXmlDump){
		disasmSetXmlOutput();
		fprintf(fp, "<html><body><pre>\n");
	}
	for(int iLoop = 0; iLoop < prx->elf.iSHCount; iLoop++){
		if(prx->elf.sections[iLoop].flags & (SHF_EXECINSTR | SHF_ALLOC)){
			if((prx->elf.sections[iLoop].iSize > 0) && (prx->elf.sections[iLoop].type == SHT_PROGBITS)){
				fprintf(fp, "\n; ==== Section %s - Address 0x%08X Size 0x%08X Flags 0x%04X\n", 
						prx->elf.sections[iLoop].szName, prx->elf.sections[iLoop].iAddr + prx->dwBase, 
						prx->elf.sections[iLoop].iSize, prx->elf.sections[iLoop].flags);
				if(prx->elf.sections[iLoop].flags & SHF_EXECINSTR){
					PrxDisasm(prx, fp, prx->elf.sections[iLoop].iAddr + prx->dwBase, 
							prx->elf.sections[iLoop].iSize, 
							(uint8_t*) VmemGetPtr(&prx->vMem, prx->elf.sections[iLoop].iAddr),
							prx->imms, prx->dwBase);
				}else{
					PrxDumpData(prx, fp, prx->elf.sections[iLoop].iAddr + prx->dwBase, 
							prx->elf.sections[iLoop].iSize,
							(uint8_t*) VmemGetPtr(&prx->vMem, prx->elf.sections[iLoop].iAddr));
					PrxDumpStrings(prx, fp, prx->elf.sections[iLoop].iAddr + prx->dwBase, 
							prx->elf.sections[iLoop].iSize, 
							(uint8_t*) VmemGetPtr(&prx->vMem, prx->elf.sections[iLoop].iAddr));
				}
			}
		}
	}
	if(prx->blXmlDump){
		fprintf(fp, "</pre></body></html>\n");
	}
	disasmSetSymbols(NULL);
}

void PrxDumpXML(CProcessPrx*prx,FILE *fp, const char *disopts){
	//disasmSetSymbols(&prx->syms);
	disasmSetOpts(disopts, 1);

	char *slash = strrchr(prx->elf.filename, '/');
	if(!slash){
		slash = prx->elf.filename;
	}else{
		slash++;
	}

	fprintf(fp, "<prx file=\"%s\" name=\"%s\">\n", slash, prx->module.info.name);
	fprintf(fp, "<exports>\n");
	for(int i;i<prx->module.exports_count;i++){
		PspEntries *pExport = &prx->module.exports[i];
		fprintf(fp, "<lib name=\"%s\">\n", pExport->name);
		for(int i = 0; i < pExport->f_count; i++)
			fprintf(fp, "<func nid=\"0x%08X\" name=\"%s\" ref=\"0x%08X\" />\n", pExport->funcs[i].nid, pExport->funcs[i].name,pExport->funcs[i].addr);
		fprintf(fp, "</lib>\n");
	}
	fprintf(fp, "</exports>\n");

	for(int iLoop = 0; iLoop < prx->elf.iSHCount; iLoop++){
		if(prx->elf.sections[iLoop].flags & (SHF_EXECINSTR | SHF_ALLOC)){
			if((prx->elf.sections[iLoop].iSize > 0) && (prx->elf.sections[iLoop].type == SHT_PROGBITS)){
				if(prx->elf.sections[iLoop].flags & SHF_EXECINSTR){
					fprintf(fp, "<disasm>\n");
					PrxDisasmXML(prx, fp, prx->elf.sections[iLoop].iAddr + prx->dwBase, prx->elf.sections[iLoop].iSize, (uint8_t*) VmemGetPtr(&prx->vMem, prx->elf.sections[iLoop].iAddr),prx->imms, prx->dwBase);
					fprintf(fp, "</disasm>\n");
				}
			}
		}
	}
	fprintf(fp, "</prx>\n");
	disasmSetSymbols(NULL);
}
