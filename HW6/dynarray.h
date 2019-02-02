/*--------------------------------------------------------------------*/
/* dynarray.h                                                         */
/* Author: Gyeongman Kim  20150073                                    */
/*--------------------------------------------------------------------*/

#ifndef DYNARRAY_INCLUDED
#define DYNARRAY_INCLUDED

/*--------------------------------------------------------------------*/
/* A DynArray consists of an array, along with its logical and
   physical lengths. */
struct DynArray
{
	/* The number of elements in the DynArray from the client's
	   point of view. */
	int iLength;

	/* The number of elements in the array that underlies the
	   DynArray. */
	int iPhysLength;

	/* The array that underlies the DynArray and this contain Token
        structure. */
	void **ppvArray;
};

/* A DynArray_T is an array whose length can expand dynamically. */
typedef struct DynArray * DynArray_T;


/* Return a new DynArray_T object whose length is iLength, or
   NULL if insufficient memory is available. */
DynArray_T DynArray_new(int iLength);

/* Free oDynArray. */
void DynArray_free(DynArray_T oDynArray);

/* Increase the physical length of oDynArray.  Return 1 (TRUE) if
   successful and 0 (FALSE) if insufficient memory is available. */
int DynArray_grow(DynArray_T oDynArray);

/* Add pvElement to the end of oDynArray, thus incrementing its length.
   Return 1 (TRUE) if successful, or 0 (FALSE) if insufficient memory
   is available. */
int DynArray_add(DynArray_T oDynArray, void *pvElement);

#endif
