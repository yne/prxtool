/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * SerializePrx.h - Definition of a class to serialize a PRX.
 ***************************************************************/

#ifndef __SERIALIZEPRX_H__
#define __SERIALIZEPRX_H__



#include "ProcessPrx.h"

enum {
	SERIALIZE_IMPORTS  = (1 << 0),
	SERIALIZE_EXPORTS  = (1 << 1),
	SERIALIZE_SECTIONS = (1 << 2),
	SERIALIZE_RELOCS   = (1 << 3),
	SERIALIZE_DOSYSLIB = (1 << 4),
	SERIALIZE_ALL	   = 0xFFFFFFFF
};

/** Base class for serializing a prx file */
class CSerializePrx {
protected:
	/** Called when the output file is started */
	virtual int StartFile()											= 0;
	/** Called when the output file is ended */
	virtual int EndFile()												= 0;
	/** Called when a new prx is about to be serialized */
	virtual int StartPrx(const char *szFilename, const PspModule *mod, uint32_t iSMask)			= 0;
	virtual int EndPrx()												= 0;
	/** Called when we are about to start serializing the sections */
	virtual int StartSects()											= 0;
	/** Called when we want to serialize a section */
	virtual int SerializeSect(int index, ElfSection &sect)				= 0;
	/** Called when have finished serializing the sections */
	virtual int EndSects()												= 0;
	virtual int StartImports()											= 0;
	virtual int SerializeImport(int index, const PspLibImport *imp)	= 0;
	virtual int EndImports()											= 0;
	virtual int StartExports()											= 0;
	virtual int SerializeExport(int index, const PspLibExport *exp)	= 0;
	virtual int EndExports()											= 0;
	virtual int StartRelocs()											= 0;
	/* Called with a list of relocs for a single segment */
	virtual int SerializeReloc(int count, const ElfReloc *rel)		= 0;
	virtual int EndRelocs()											= 0;

	/** Pointer to the current prx, if the functions need it for what ever reason */
	CProcessPrx* m_currPrx;
	int m_blStarted;

	void DoSects(CProcessPrx &prx);
	void DoImports(CProcessPrx &prx);
	void DoExports(CProcessPrx &prx, int blDoSyslib);
	void DoRelocs(CProcessPrx &prx);
public:
	CSerializePrx();
	virtual ~CSerializePrx();
	int Begin();
	int SerializePrx(CProcessPrx &prx, uint32_t iSMask);
	int End();
};

#endif
