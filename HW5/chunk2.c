/*--------------------------------------------------------------------*/
/* chunk2.c                                                           */
/* Author: Gyeong man Kim   20150073                                  */
/* Modified by Younghwan Go and KyoungSoo Park                        */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>

#include "chunk2.h"

enum {
    MEMALLOC_MIN = 1024,
};

/* gHeapStart, gHeapEnd: start and end of the heap area.
 * gHeapEnd will move if you increase the heap */
static void *gHeapStart = NULL, *gHeapEnd = NULL;

/*--------------------------------------------------------------------*/
/* Initialize gHeapStart if it is not initialized yet.
   terminate program if fail to function sbrk(0).*/
void
ChunkInitDS(void)
{
    gHeapStart = gHeapEnd = sbrk(0);
    if (gHeapStart == (void *)-1) {
        fprintf(stderr, "sbrk(0) failed\n");
        exit(-1);
    }
}
/*--------------------------------------------------------------------*/
/* If a chunk is not in the free chunk list, then the next pointer
    * must be NULL. So we can figure out whether a chunk is in use or
    * free by looking at the next chunk pointer. */
int
ChunkGetStatus(Chunk_T c)
{
    if (c->next == NULL)
        return CHUNK_IN_USE;
    else
        return CHUNK_FREE;
}
/*--------------------------------------------------------------------*/
/* Chunk in Free List connected each other with next member.
   so this function return address of next chunk in free list*/
Chunk_T
ChunkNextFreeChunk(Chunk_T c)
{
    return c->next;
}
/*--------------------------------------------------------------------*/
/* We get address of chunk which go to free list by parameter
   decide bins' array index by c->units and If there is no chunk
   in list, put c in free list and connect itself.
   If there is some chunk, put c in free list in first and set c
   gFreeHead. And connect with original gFreeHead chunk by using
   footer's prev. In this case, free list's gFreeTail is pointing
   itself for next chunk. This is way of know what is gFreeTail chunk.*/
void
ChunkInsertFirst(Chunk_T c)
{
    assert (ChunkGetStatus(c) != CHUNK_FREE);
    assert (c->units >= 1);

    Footer_T f = (Footer_T)(c + c->units + 1);
    int j = c->units;
    if (j > 1023) j = 1023; //if units>1023, put in bins[2013]

   /* If free chunk list is empty, chunk c points to itself. */
    if (bins[j] == NULL) {
        bins[j] = c;
        c->next = c;
        f->units = c->units;
        f->prev = c;
    }
    else {
        c->next = bins[j]; /* now c is CHUNK_FREE */
        Footer_T g = (Footer_T)(bins[j] + bins[j]->units + 1);
        f->units = c->units;
        f->prev = c;   //gFreeHead's prev chunk is itself
        g->prev = c;
        bins[j] = c;
    }
}

/*--------------------------------------------------------------------*/
/* Chunk in Free List have information of units.
   so this function return amount of units in this chunk*/
int
ChunkGetUnits(Chunk_T c)
{
    return c->units;
}
/*--------------------------------------------------------------------*/
/* There is many chunks in memory heap.
   so this function return address of next adjacent chunk in memory*/
Chunk_T
ChunkNextAdjacent(Chunk_T c)
{
    Chunk_T n;

   /* Note that a chunk consists of one chunk unit for a header and
    * many chunk units for data and one chunk unit for a footer. */
    n = c + c->units + 2;

   /* If 'c' is the last chunk in memory space, then return NULL. */
    if ((void *)n >= gHeapEnd)
        return NULL;

    return n;
}
/*--------------------------------------------------------------------*/
/* There is many chunks in memory heap.
   so function return address of previous adjacent chunk in memory*/
Chunk_T
ChunkPrevAdjacent(Chunk_T c)
{
    Chunk_T n;

    Footer_T f = (Footer_T)(c - 1);  //address of prev chunk's footer

    if ((void *)(f) <= gHeapStart) return NULL;


   /* Note that a chunk consists of one chunk unit for a header and
    * many chunk units for data and one chunk unit for a footer. */
    n = (Chunk_T)(f - f->units - 1);

    return n;
}
/*--------------------------------------------------------------------*/
/* This function is used in ChunkFreeChunk(). When there are two
   Chunk in use adjacent, merge them and return first chunk address */
Chunk_T
ChunkMerge(Chunk_T c1, Chunk_T c2)
{
   /* c1 and c2 must be be adjacent */
   assert (c1 < c2);
   assert (ChunkNextAdjacent(c1) == c2);
   assert (ChunkGetStatus(c1) == CHUNK_IN_USE);
   assert (ChunkGetStatus(c2) == CHUNK_IN_USE);

   /* Adjust the merged chunk */
   Footer_T f = (Footer_T)(c2 + c2->units + 1);
   c1->units += c2->units + 2;
   f->units = c1->units;
   /*Do not control chunk's next or prev*/

   return c1;
}
/*--------------------------------------------------------------------*/
/* This function is used in mymalloc(). When there are freechunk which
   size is bigger than we want, we call this function to split one chunk
   to two. So malloc c2 and move c in another index free list.
   This function return c2*/
Chunk_T
ChunkSplit(Chunk_T c, int units)
{
   Chunk_T c2;

   assert (c >= (Chunk_T)gHeapStart && c <= (Chunk_T)gHeapEnd);
   assert (ChunkGetStatus(c) == CHUNK_FREE);
   assert (c->units > units + 2); /* assume  a header and footer chunk*/

   ChunkRemoveFromListNew(c);
   /* prepare for the second chunk */
   c2 = c + c->units - units;
   c2->units = units;
   Footer_T g = (Footer_T)(c2 + c2->units + 1);
   g->units = c2->units;
   g->prev = NULL;
   c2->next = NULL;/* set next to NULL since it will be used soon */


   /* the original chunk size should be decreased by c2's size */
   c->units -= (units + 2);
   Footer_T f = (Footer_T)(c + c->units + 1);
   f->units = c->units;
   ChunkInsertFirst(c); //move c to another index free list
   return c2;
}
/*--------------------------------------------------------------------*/
/* This function is used in ChunkFreeChunk(). When there is free chunk
   which adjacent with chunk that want to free, first, remove chunk in
   free list and merge them. So this function connect each other that
   next chunk and prev chunk in free list and set next is NULL. */
void
ChunkRemoveFromListNew(Chunk_T c)
{
    assert (ChunkGetStatus(c) == CHUNK_FREE);

    Footer_T f = (Footer_T)(c + c->units + 1);
    Footer_T g = (Footer_T)(c->next + c->next->units + 1);
    int j = c->units;
    if (j > 1023) j = 1023; //find index

    if (f->prev == c) {
        if (c->next == c) {
            bins[j] = NULL;
        }
        else {
            g->prev = c->next;
            bins[j] = c->next;
        }
      /* c is the only chunk in the free list */
    }
    /* If free chunk list is some chunks, remove chunk c in free list.*/
    else {
        if (c->next == c) {
            f->prev->next = f->prev;
        }
        else {
            f->prev->next = c->next;
            g->prev = f->prev;
        }
    }
    c->next = NULL; /* its status is now CHUNK_IN_USE */
    f->prev = NULL;
    return;
}
/*--------------------------------------------------------------------*/
/* When this function called, we can call ChunkRemoveFromListNew()
   because we can know previous chunk by using footer. */
void
ChunkRemoveFromList(Chunk_T prev, Chunk_T c)
{
   ChunkRemoveFromListNew(c);
   return;
}
/*--------------------------------------------------------------------*/
/* This function is used in mymalloc, so when there is no memory,
   call this function and allocate memory units+2 unit size. And,
   if previous adjacent chunk is free, we should merge. */
Chunk_T
ChunkAllocateMem(int units)
{
    Chunk_T c, n;
    assert (units > 0);

    if (units < MEMALLOC_MIN)
        units = MEMALLOC_MIN;

   /*we need to allocate two more unit for header and footer. */
    c = (Chunk_T)sbrk((units + 2) * CHUNK_UNIT);
    if (c == (Chunk_T)-1)
        return NULL;

    gHeapEnd = sbrk(0);

    c->units = units;
    Footer_T f = (Footer_T)(c + c->units + 1);
    f->units = units;

    c->next = NULL;  //set chunk in use for merge
    f->prev = NULL;

   /* Insert the newly allocated chunk 'c' to the free list.
    * Note that the list is sorted in ascending order. */

    if ((n = ChunkPrevAdjacent(c)) != NULL
                    && ChunkGetStatus(n) == CHUNK_FREE) {
        ChunkRemoveFromListNew(n);
        c = ChunkMerge(n, c);
    }

    ChunkInsertFirst(c);
    /* Insert the c to the appropriate index free list.*/

    assert(ChunkValidityCheck());

    return c;
}
/*--------------------------------------------------------------------*/
/* When we want to free chunk c, first, look the next and previous chunk
   of c and if their are free chunk, merge it. Then, call
   ChunkInsertFirstInsert() for insert c to the free list.*/
void
ChunkFreeChunk(Chunk_T c)
{
    Chunk_T n, m;

    /*look their are free list*/
    if ((n = ChunkPrevAdjacent(c)) != NULL
                        && ChunkGetStatus(n) == CHUNK_FREE) {
        ChunkRemoveFromListNew(n);
        c = ChunkMerge(n, c);
    }
    if ((m = ChunkNextAdjacent(c)) != NULL
                        && ChunkGetStatus(m) == CHUNK_FREE) {
        ChunkRemoveFromListNew(m);
        c = ChunkMerge(c, m);
    }
    ChunkInsertFirst(c); //insert in free list
    return ;
}

#ifndef NDEBUG
/*--------------------------------------------------------------------*/
/* Chunk should be in memory heap and has units more than 0. */
static int
ChunkIsValid(Chunk_T c)
/* Return 1 (TRUE) iff c is valid */
{
    assert(c != NULL);
    assert(gHeapStart != NULL);
    assert(gHeapEnd != NULL);
    Footer_T f = (Footer_T)(c + c->units + 1);

    if (c < (Chunk_T)gHeapStart)
        {fprintf(stderr, "Bad heap start\n"); return 0; }
    if (c >= (Chunk_T)gHeapEnd)
        {fprintf(stderr, "Bad heap end\n"); return 0; }
    if (c->units == 0 || f->units == 0)
        {fprintf(stderr, "Zero units\n"); return 0; }
    if (c->units < 0 || f->units < 0)
        {fprintf(stderr, "bad units\n"); return 0; }
    return 1;
}

/*--------------------------------------------------------------------*/
/* In this function, check the all chunk in memory heap that they are
   valid. for example, chunk in free list should not c->next value
   is NULL.*/
int
ChunkValidityCheck(void)
{
    Chunk_T w, n;
    int j;

    for (j=0; j<1024; j++) {
        if (gHeapStart == gHeapEnd) {
            if (bins[j] != NULL) {
                fprintf(stderr, "Inconsistent empty heap\n");
                return 0;
            }
        }
    }

    for (w = (Chunk_T)gHeapStart;
            w && w < (Chunk_T)gHeapEnd;
            w = ChunkNextAdjacent(w)) {
        if (!ChunkIsValid(w))
            return 0;
    }

    /*Check all chunk in all free list in bins[]*/
    for (j=0; j<1024; j++) {
        for (w = bins[j]; w; w = w->next) {
            Footer_T f = (Footer_T)(w + w->units + 1);
            if (ChunkGetStatus(w) != CHUNK_FREE) {
                fprintf(stderr,
                            "Non-free chunk in the free chunk list\n");
                return 0;
            }
            if (!ChunkIsValid(w))
                return 0;
            if ((n = ChunkNextAdjacent(w)) != NULL) {
                if (ChunkGetStatus(n) == CHUNK_FREE) {
                    fprintf(stderr, "Uncoalesced chunks\n");
                    return 0;
                }
            }
            if ((n = ChunkPrevAdjacent(w)) != NULL) {
                if (ChunkGetStatus(n) == CHUNK_FREE) {
                    fprintf(stderr, "Uncoalesced chunks\n");
                    return 0;
                }
            }
            if (f->prev == NULL) {
                fprintf(stderr,
                            "Non-free chunk in the free chunk list\n");
                return 0;
            }
            if (w->units != j) {
                if (w->units < 1024) {
                    fprintf(stderr, "Wrong index in bins\n");
                    return 0;
                }
            }
            if (w->next == w) break;
        }
    }
    return 1;
}

/*--------------------------------------------------------------------*/
/* This function print chunk's information*/
void
ChunkPrint(void)
{
    Chunk_T w;
    int j;

    printf("heap: %p ~ %p\n", gHeapStart, gHeapEnd);

    printf("free chain\n");
    for (j=0; j<1024; j++) {
        for (w = bins[j]; w; w = w->next) {
            printf("bins index : %d\n", j);
            Footer_T f = (Footer_T)(w + w->units + 1);
            if (f->prev == w) {
                printf("%p is the gFreeHead of this index!\n",
                        (void *)f->prev); exit(0);
            }
            printf("[%p: %ld units]\n", (void *)w, w->units);

            if (w->next == w) break;
        }
    }

    printf("mem\n");
    for (w = (Chunk_T)gHeapStart;
            w < (Chunk_T)gHeapEnd;
            w = (Chunk_T)((char *)w + (w->units + 2) * CHUNK_UNIT))
        printf("[%p (%c): %ld units]\n",
                (void *)w, w->next ? 'F' : 'U', w->units);
}
#endif
