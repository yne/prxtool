/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * SerializePrxToXml.h - Definition of a class to serialize a
 * PRX to an XML file.
 ***************************************************************/

#ifndef __SERIALIZEPRXTOXML_H__
#define __SERIALIZEPRXTOXML_H__

#include <stdio.h>
#include "SerializePrx.h"

class CSerializePrxToXml : public CSerializePrx{
	FILE *m_fpOut;

	virtual int StartFile();
	virtual int EndFile();
	virtual int StartPrx(const char *szFilename, const PspModule *mod, uint32_t iSMask);
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
	CSerializePrxToXml(FILE *fpOut);
	~CSerializePrxToXml();
};

#endif
