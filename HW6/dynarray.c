/*--------------------------------------------------------------------*/
/* dynarray.c                                                         */
/* Author: Gyeongman Kim  20150073                                    */
/*--------------------------------------------------------------------*/

#include "dynarray.h"
#include <assert.h>
#include <stdlib.h>

enum { MIN_PHYS_LENGTH = 2 };
enum { GROWTH_FACTOR = 2 };
/*--------------------------------------------------------------------*/
/* Return a new DynArray_T object whose length is iLength, or
   NULL if insufficient memory is available. */
DynArray_T DynArray_new(int iLength)
{
	DynArray_T oDynArray;

	assert(iLength >= 0);

	oDynArray = (struct DynArray*)malloc(sizeof(struct DynArray));
	if (oDynArray == NULL)
		return NULL;

	oDynArray->iLength = iLength;
	if (iLength > MIN_PHYS_LENGTH)
		oDynArray->iPhysLength = iLength;
	else
		oDynArray->iPhysLength = MIN_PHYS_LENGTH;

	oDynArray->ppvArray =
		(void**)calloc((size_t)oDynArray->iPhysLength,
							 sizeof(void*));
	if (oDynArray->ppvArray == NULL) {
		free(oDynArray);
		return NULL;
	}

	return oDynArray;
}
/*--------------------------------------------------------------------*/
/* Free oDynArray. */
void DynArray_free(DynArray_T oDynArray)
{
	assert(oDynArray != NULL);

	free(oDynArray->ppvArray);
	free(oDynArray);
}
/*--------------------------------------------------------------------*/
/* Increase the physical length of oDynArray.  Return 1 (TRUE) if
   successful and 0 (FALSE) if insufficient memory is available. */
int DynArray_grow(DynArray_T oDynArray)
{
	int iNewLength;
    void **ppvNewArray;

	assert(oDynArray != NULL);

	iNewLength = oDynArray->iPhysLength * GROWTH_FACTOR;

	ppvNewArray = (void**)
		realloc(oDynArray->ppvArray, sizeof(void*) * iNewLength);
	if (ppvNewArray == NULL)
		return 0;

	oDynArray->iPhysLength = iNewLength;
	oDynArray->ppvArray = ppvNewArray;

	return 1;
}
/*--------------------------------------------------------------------*/
/* Add pvElement to the end of oDynArray, thus incrementing its length.
   Return 1 (TRUE) if successful, or 0 (FALSE) if insufficient memory
   is available. */
int DynArray_add(DynArray_T oDynArray,  void *pvElement)
{
	assert(oDynArray != NULL);

	if (oDynArray->iLength == oDynArray->iPhysLength)
		if (!DynArray_grow(oDynArray))
			return 0;

	oDynArray->ppvArray[oDynArray->iLength] = pvElement;
	oDynArray->iLength++;

	return 1;
}
