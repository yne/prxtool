/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * SerializePrxToMap.h - Implementation of a class to serialize
 * a loaded PRX file to a PS2DIS Map file.
 ***************************************************************/

#ifndef __SERIALIZEPRXTOMAP_H__
#define __SERIALIZEPRXTOMAP_H__

#include <stdio.h>
#include "SerializePrx.h"

class CSerializePrxToMap : public CSerializePrx{
	FILE *m_fpOut;

	virtual int StartFile();
	virtual int EndFile();
	virtual int StartPrx(const char *szFilename, const PspModule *pMod, uint32_t iSMask);
	virtual int EndPrx();
	virtual int StartSects();
	virtual int SerializeSect(int num, ElfSection &sect);
	virtual int EndSects();
	virtual int StartImports();
	virtual int SerializeImport(int num, const PspLibImport *imp);
	virtual int EndImports();
	virtual int StartExports();
	virtual int SerializeExport(int num, const PspLibExport *exp);
	virtual int EndExports();
	virtual int StartRelocs();
	virtual int SerializeReloc(int count, const ElfReloc *rel);
	virtual int EndRelocs();

public:
	CSerializePrxToMap(FILE *fpOut);
	~CSerializePrxToMap();
};

#endif
