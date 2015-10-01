/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * VirtualMem.C - An implementation of a class to virtualise
 * a memory space.
 ***************************************************************/

typedef enum{
	MEM_LITTLE_ENDIAN = 0,
	MEM_BIG_ENDIAN = 1
}MemEndian;

typedef struct{
	uint8_t *m_pData;
	uint32_t m_iSize;
	int32_t m_iBaseAddr;
	MemEndian m_endian;
}Vmem;

#define CHECK_ADDR(mem, addr, size) ((mem->m_pData) && (addr >= mem->m_iBaseAddr)&&((addr + size) < (mem->m_iBaseAddr + mem->m_iSize)))
#include "endianness.c"

uint8_t VmemGetU8(Vmem*mem, uint32_t iAddr){
	if(!CHECK_ADDR(mem,iAddr, 1))
		return fprintf(stdout,"Invalid memory address 0x%08X\n", iAddr),0;
	return mem->m_pData[iAddr - mem->m_iBaseAddr];
}

uint16_t VmemGetU16(Vmem*mem, uint32_t iAddr){
	if(!CHECK_ADDR(mem,iAddr, 2))
		return fprintf(stdout,"Invalid memory address 0x%08X\n", iAddr),0;
	if(mem->m_endian == MEM_LITTLE_ENDIAN)
		return LH_LE(*((uint16_t*) &mem->m_pData[iAddr - mem->m_iBaseAddr]));
	if(mem->m_endian == MEM_BIG_ENDIAN)
		return LH_BE(*((uint16_t*) &mem->m_pData[iAddr - mem->m_iBaseAddr]));
	return fprintf(stdout,"Invalid endian format\n"),0;
}

uint32_t   VmemGetU32(Vmem*mem, uint32_t iAddr){
	if(!CHECK_ADDR(mem,iAddr, 4))
		return fprintf(stdout,"Invalid memory address 0x%08X\n", iAddr),0;
	if(mem->m_endian == MEM_LITTLE_ENDIAN)
		return LW_LE(*((uint32_t*) &mem->m_pData[iAddr - mem->m_iBaseAddr]));
	if(mem->m_endian == MEM_BIG_ENDIAN)
		return LW_BE(*((uint32_t*) &mem->m_pData[iAddr - mem->m_iBaseAddr]));
	return fprintf(stdout,"Invalid endian format\n"),0;
}

int8_t    VmemGetS8(Vmem*mem, uint32_t iAddr){
	if(!CHECK_ADDR(mem,iAddr, 1))
		return fprintf(stdout,"Invalid memory address 0x%08X\n", iAddr),0;
	return mem->m_pData[iAddr - mem->m_iBaseAddr];
}

int16_t   VmemGetS16(Vmem*mem, uint32_t iAddr){
	if(!CHECK_ADDR(mem,iAddr, 2))
		return fprintf(stdout,"Invalid memory address 0x%08X\n", iAddr),0;
	if(mem->m_endian == MEM_LITTLE_ENDIAN)
		return LH_LE(*((uint16_t*) &mem->m_pData[iAddr - mem->m_iBaseAddr]));
	if(mem->m_endian == MEM_BIG_ENDIAN)
		return LH_BE(*((uint16_t*) &mem->m_pData[iAddr - mem->m_iBaseAddr]));
	return fprintf(stdout,"Invalid endian format\n"),0;
}

int32_t   VmemGetS32(Vmem*mem, uint32_t iAddr){
	if(!CHECK_ADDR(mem,iAddr, 4))
		return fprintf(stdout,"Invalid memory address 0x%08X\n", iAddr),0;
	if(mem->m_endian == MEM_LITTLE_ENDIAN)
		return LW_LE(*((uint32_t*) &mem->m_pData[iAddr - mem->m_iBaseAddr]));
	if(mem->m_endian == MEM_BIG_ENDIAN)
		return LW_BE(*((uint32_t*) &mem->m_pData[iAddr - mem->m_iBaseAddr]));
	return fprintf(stdout,"Invalid endian format\n"),0;
}

void *VmemGetPtr(Vmem*mem, uint32_t iAddr){
	if(CHECK_ADDR(mem,iAddr, 1))
		return fprintf(stdout,"Ptr out of region 0x%08X\n", iAddr),NULL;
	return &mem->m_pData[iAddr - mem->m_iBaseAddr];
}

/* Get the amount of data available from this address */
uint32_t VmemGetSize(Vmem*mem, uint32_t iAddr){
	/* Check we have at least 1 byte left */
	if(!CHECK_ADDR(mem,iAddr, 1))
		return 0;
	return mem->m_iSize - (iAddr - mem->m_iBaseAddr);
}

uint32_t VmemCopy(Vmem*mem, void *pDest, uint32_t iAddr, uint32_t iSize){
	uint32_t size = VmemGetSize(mem,iAddr);
	uint32_t iCopySize = size > iSize ? iSize : size;
	void *ptr;
	if(iCopySize > 0)
		memcpy(VmemGetPtr(mem,iAddr), ptr, iCopySize);
	return iCopySize;
}