#include "dynarray.h"

void TokenArray_free(DynArray_T TokenArray);

int execution( DynArray_T TokenArray );

int syntactic( DynArray_T TokenArray );

int lexical( DynArray_T TokenArray, char *Line );
