/*--------------------------------------------------------------------*/
/* heapmgr2.c                                                         */
/* Author: Gyeongman Kim   20150073                                   */
/* Modified by Younghwan Go and KyoungSoo Park                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <assert.h>

#include "chunk2.h"

/*--------------------------------------------------------------*/
/* SizeToUnits:
 * Returns capable number of units for 'size' bytes. */
/*--------------------------------------------------------------*/
int
SizeToUnits(int size)
{
   return (size + (CHUNK_UNIT-1))/CHUNK_UNIT;
}

/*--------------------------------------------------------------*/
/* GetChunkFromDataPtr:
 * Returns the header pointer that contains data 'm'. */
/*--------------------------------------------------------------*/
Chunk_T
GetChunkFromDataPtr(void *m)
{
	return (Chunk_T)((char *)m - CHUNK_UNIT);
}

/*--------------------------------------------------------------*/
/* mymalloc:
 * Dynamically allocate a memory capable of holding size bytes.
 * Substitute for GNU malloc().                                 */
/*--------------------------------------------------------------*/
void *
mymalloc(size_t size)
{
    static int isInitialized = 0;
    Chunk_T c;
    int units, j, k;

    if (size <= 0)
        return NULL;

    if (isInitialized == 0) {
        ChunkInitDS();      //initialize
        isInitialized = 1;
    }

    assert(ChunkValidityCheck());

    units = SizeToUnits(size);
    j = units;
    if (j > 1023) j = 1023;  //decide index



    /*if j=1023, they want memory more than 1023 so allocate and
      malloc it in bins[1023]*/
    if (j == 1023) {
        if ((c = ChunkAllocateMem(units)) == NULL) {
            assert (0); /* should not happen */
            return NULL;
        }
        for (c = bins[1023]; c; c = c->next) {
            if (ChunkGetUnits(c) >= units) {
                if (ChunkGetUnits(c) > units + 2) {
                    c = ChunkSplit(c, units);
                }
                else {
                    ChunkRemoveFromListNew(c);
                }
                assert(ChunkValidityCheck());
                return ((char *)c + CHUNK_UNIT);
            }
            if (c->next == c) break; //Unreachable here
        }

    }

    /*j < 1023*/
    else {
        if (bins[j] == NULL) {
            for (k=j+1; k<1024; k++) {  //traversing until find chunk
                if (bins[k] != NULL) {   //find
                    c = bins[k];
                    if (ChunkGetUnits(c) > units + 2) {
                        c = ChunkSplit(c, units);
                    }
                    else {
                        ChunkRemoveFromListNew(c);
                    }
                    assert(ChunkValidityCheck());
                    return ((char *)c + CHUNK_UNIT);
                }
                if (k == 1023) {  //when fail to find
                    if ((c = ChunkAllocateMem(units)) == NULL) {
                        assert (0); /* should not happen */
                        return NULL;
                    }
                    c = bins[1023];
                    if (ChunkGetUnits(c) > units + 2) {
                        c = ChunkSplit(c, units);
                    }
                    else {
                        ChunkRemoveFromListNew(c);
                    }
                    assert(ChunkValidityCheck());
                    return ((char *)c + CHUNK_UNIT);
                }
            }
        }

        else {          //use chunk in bins[j] directly
            c = bins[j];
            ChunkRemoveFromListNew(c);

            assert(ChunkValidityCheck());
            return ((char *)c + CHUNK_UNIT);
        }
    }
   /* Unreachable here */
    return NULL;
}

/*--------------------------------------------------------------*/
/* myfree:
 * Releases dynamically allocated memory.
 * Substitute for GNU free().                                   */
/*--------------------------------------------------------------*/
void
myfree(void *m)
{
   Chunk_T c;

   if (m == NULL)
      return;

   /* Check everything is OK before freeing 'm' */
   assert(ChunkValidityCheck());

   /* Get the chunk header pointer from m */
   c = GetChunkFromDataPtr(m);
   assert (ChunkGetStatus(c) != CHUNK_FREE);

   /* Insert the chunk into the free chunk list */
   ChunkFreeChunk(c);

   /* Check everything is OK after freeing 'm' */
   assert(ChunkValidityCheck());
}
