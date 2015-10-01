/***************************************************************
 * PRXTool : Utility for PSP executables.
 * (c) TyRaNiD 2k5
 *
 * SerializePrx.C - Implementation of a class to serialize a 
 * loaded PRX.
 ***************************************************************/

#include <stdio.h>
#include <string.h>
#include "SerializePrx.h"
#include "output.h"

CSerializePrx::CSerializePrx(){
	m_blStarted = 0;
}

CSerializePrx::~CSerializePrx(){
}

void CSerializePrx::DoSects(CProcessPrx &prx){
	ElfSection *pSections;
	uint32_t iSects;
	uint32_t iLoop;

	if(StartSects() == 0){
		throw 0;
	}

	/* We already checked for NULL */
	pSections = prx.ElfGetSections(iSects);
			
	for(iLoop = 0; iLoop < iSects; iLoop++){
		if(SerializeSect(iLoop, pSections[iLoop]) == 0){
			throw 0;
		}
	}

	if(EndSects() == 0){
		throw 0;
	}
}

void CSerializePrx::DoImports(CProcessPrx &prx){
	PspModule *pMod;
	PspEntries *pImport;
	int iLoop;

	pMod = prx.GetModuleInfo();
	iLoop = 0;

	if(StartImports() == 0){
		throw 0;
	}

	pImport = pMod->imports;
	while(pImport != NULL){
		if(SerializeImport(iLoop, pImport) == 0){
			throw 0;
		}

		iLoop++;
		pImport = pImport->next;
	}

	if(EndImports() == 0){
		throw 0;
	}
}

void CSerializePrx::DoExports(CProcessPrx &prx, int blDoSyslib){
	PspModule *pMod;
	PspEntries *pExport;
	int iLoop;

	pMod = prx.GetModuleInfo();
	iLoop = 0;

	if(StartExports() == 0){
		throw 0;
	}

	pExport = pMod->exports;
	while(pExport != NULL){
		if((blDoSyslib) || (strcmp(pExport->name, PSP_SYSTEM_EXPORT) != 0)){
			if(SerializeExport(iLoop, pExport) == 0){
				throw 0;
			}
			iLoop++;
		}

		pExport = pExport->next;
	}

	if(EndExports() == 0){
		throw 0;
	}
}

void CSerializePrx::DoRelocs(CProcessPrx &prx){
	ElfReloc* pRelocs;
	int iCount;

	if(StartRelocs() == 0){
		throw 0;
	}
			
	pRelocs = prx.GetRelocs(iCount);
	if(pRelocs != NULL){
		/* Process the relocs a segment at a time */
		const char *pCurrSec;
		int iCurrCount;

		while(iCount > 0){
			ElfReloc *pBase;

			pBase = pRelocs;
			pCurrSec = pRelocs->secname;
			iCurrCount = 0;
			while((iCount > 0) && (pCurrSec == pRelocs->secname || (pCurrSec != NULL && pRelocs->secname != NULL && (strcmp(pCurrSec, pRelocs->secname) == 0)))){
				pRelocs++;
				iCurrCount++;
				iCount--;
			}

			if(iCurrCount > 0){
				if(SerializeReloc(iCurrCount, pBase) == 0){
					throw 0;
				}
			}
		}
	}

	if(EndRelocs() == 0){
		throw 0;
	}
}

int CSerializePrx::Begin(){
	if(StartFile() == 0){
		return 0;
	}

	m_blStarted = 1;

	return 1;
}

int CSerializePrx::End(){
	int blRet = 1;
	if(m_blStarted == 1){
		blRet = EndFile();
		m_blStarted = 0;
	}

	return blRet;
}

int CSerializePrx::SerializePrx(CProcessPrx &prx, uint32_t iSMask){
	int blRet = 0;

	if(m_blStarted == 0){
		if(Begin() != 1){
			fprintf(stderr, "Failed to begin the serialized output");
			return 0;
		}
	}

	try{
		/* Let's check the prx so we don't have to in the future */
		PspModule *pMod;
		uint32_t iSectNum;

		m_currPrx = &prx;
		pMod = prx.GetModuleInfo();
		if(pMod == NULL){
			fprintf(stderr, "Invalid module info pMod\n");
			throw 0;
		}

		if((prx.ElfGetSections(iSectNum) == NULL) && (iSectNum > 0)){
			fprintf(stderr, "Invalid section header information\n");
			throw 0;
		}

		if(StartPrx(prx.GetElfName(), pMod, iSMask) == 0){
			throw 0;
		}

		if(iSMask & SERIALIZE_SECTIONS){
			DoSects(prx);
		}

		if(iSMask & SERIALIZE_IMPORTS){
			DoImports(prx);
		}

		if(iSMask & SERIALIZE_EXPORTS){
			DoExports(prx, iSMask & SERIALIZE_DOSYSLIB ? 1 : 0);
		}

		if(iSMask & SERIALIZE_RELOCS){
			DoRelocs(prx);
		}

		if(EndPrx() == 0){
			throw 0;
		}
	}
	catch(...){
		/* Do nothing */
	}

	return blRet;
}
