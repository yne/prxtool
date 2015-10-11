#include "db_func.c"
#include "db_nids.c"

typedef struct{
	size_t        nids_count;
	LibraryNid*   nids;
	size_t        libraries_count;
	LibraryEntry* libraries;
	size_t        functions_count;
	FunctionType* functions;
}DataBase;


