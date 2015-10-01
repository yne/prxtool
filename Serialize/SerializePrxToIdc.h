/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * SerializePrxToIdc.h - Definition of a class to serialize a 
 * PRX to an IDA Pro IDC file.
 ***************************************************************/

#ifndef __SERIALIZEPRXTOIDC_H__
#define __SERIALIZEPRXTOIDC_H__

#include <stdio.h>
#include "SerializePrx.h"

class CSerializePrxToIdc : public CSerializePrx{
	FILE *m_fpOut;

	virtual int StartFile();
	virtual int EndFile();
	virtual int StartPrx(const char *szFilename, const PspModule *pMod, uint32_t iSMask);
	virtual int EndPrx();
	virtual int StartSects();
	virtual int SerializeSect(int num, ElfSection &sect);
	virtual int EndSects();
	virtual int StartImports();
	virtual int SerializeImport(int num, const PspEntries *imp);
	virtual int EndImports();
	virtual int StartExports();
	virtual int SerializeExport(int num, const PspEntries *exp);
	virtual int EndExports();
	virtual int StartRelocs();
	virtual int SerializeReloc(int count, const ElfReloc *rel);
	virtual int EndRelocs();

public:
	CSerializePrxToIdc(FILE *fpOut);
	~CSerializePrxToIdc();
};

#endif
