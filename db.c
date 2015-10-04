#include "db_func.c"
#include "db_nids.c"

typedef struct{
	unsigned      nids_count;
	LibraryNid*   nids;
	unsigned      libraries_count;
	LibraryEntry* libraries;
	unsigned      functions_count;
	FunctionType* functions;
}DataBase;


