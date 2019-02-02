/*--------------------------------------------------------------------*/
/* heapmgrbase.c                                                      */
/* Author: Donghwi Kim                                                */
/* Modified by Younghwan Go and KyoungSoo Park                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <assert.h>

#include "chunkbase.h"

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
   Chunk_T c, prev, pprev;
   int units;

   if (size <= 0)
      return NULL;

   if (isInitialized == 0) {
      ChunkInitDS();
      isInitialized = 1;
   }

   assert(ChunkValidityCheck());

   units = SizeToUnits(size);

   /* Traverse a free chunk list to find the first chunk whose size >=
      units */
   if ((c = ChunkFirstFreeChunk()) == NULL) {
      /* if there is no free memory chunk, allocate a new chunk */
      if ((c = ChunkAllocateMem(units)) == NULL) {
         assert (0); /* should not happen */
         return NULL;
      }
   }
   prev = ChunkLastFreeChunk(); /* prev is the tail of the list */

   while (1) {

      /* Do we have a chunk big enough for the size? */
      if (ChunkGetUnits(c) >= units) {
         if (ChunkGetUnits(c) > units + 2)  /* for header and footer */
            c = ChunkSplit(c, units);
         else
            ChunkRemoveFromList(prev, c);

         assert(ChunkValidityCheck());
         return ((char *)c + CHUNK_UNIT);
      }

      /* Move on to the next free chunk */
      pprev = prev;
      prev = c;
      c = ChunkNextFreeChunk(c);

      /* If the next chunk is the first chunk, we couldn't find a big
         enough chunk in the current list.  Try allocating a new
         memory chunk and retry here */
      if (c == ChunkFirstFreeChunk()) { /* wrap around? */
         if ((c = ChunkAllocateMem(units)) == NULL) {
            assert(0); /* should not happen */
            return NULL;
         }

         /* If the newly-allocated memory is merged with the last
            chunk, 'prev' should point to the previous chunk of the
            previous chunk (pprev) */
         if (c == prev)
            prev = pprev;
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
