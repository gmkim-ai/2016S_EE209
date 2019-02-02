/*--------------------------------------------------------------------*/
/* testheapmgr.c                                                      */
/* Author: Bob Dondero                                                */
/*--------------------------------------------------------------------*/

#include "heapmgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>

#ifndef __USE_MISC
#define __USE_MISC
#endif
#include <unistd.h>

enum {FALSE, TRUE};

/*--------------------------------------------------------------------*/

/* These arrays are too big for the stack section, so store
   them in the bss section. */

/* The maximum allowable number of calls of mymalloc(). */
enum {MAX_CALLS = 1000000};

/* Memory chunks allocated by mymalloc(). */
static char *apcChunks[MAX_CALLS] = {NULL};

/* Randomly generated chunk sizes.  */
static int aiSizes[MAX_CALLS];

/*--------------------------------------------------------------------*/

/* Function declarations. */

static void getArgs(int argc, char *argv[],
   int *piTestNum, int *piCount, int *piSize);
static void setCpuLimit(void);
static void testLifoFixed(int iCount, int iSize);
static void testFifoFixed(int iCount, int iSize);
static void testLifoRandom(int iCount, int iSize);
static void testFifoRandom(int iCount, int iSize);
static void testRandomFixed(int iCount, int iSize);
static void testRandomRandom(int iCount, int iSize);
static void testWorst(int iCount, int iSize);

/*--------------------------------------------------------------------*/

/* apcTestName is an array containing the names of the tests. */

static char *apcTestName[] =
{
   "LifoFixed", "FifoFixed", "LifoRandom", "FifoRandom",
   "RandomFixed", "RandomRandom", "Worst"
};

/*--------------------------------------------------------------------*/

/* apfTestFunction is an array containing pointers to the test
   functions.  Each pointer corresponds, by position, to a test name
   in apcTestName. */

typedef void (*TestFunction)(int, int);
static TestFunction apfTestFunction[] =
{
   testLifoFixed, testFifoFixed, testLifoRandom, testFifoRandom,
   testRandomFixed, testRandomRandom, testWorst
};

/*--------------------------------------------------------------------*/

int main(int argc, char *argv[])

/* Test the mymalloc() and myfree() functions.

   argv[1] indicates which test to run:
      LifoFixed: LIFO with fixed size chunks,
      FifoFixed: FIFO with fixed size chunks,
      LifoRandom: LIFO with random size chunks,
      FifoRandom: FIFO with random size chunks,
      RandomFixed: random order with fixed size chunks,
      RandomRandom: random order with random size chunks,
      Worst: worst case for single linked list implementation.

   argv[2] is the number of calls of mymalloc() and myfree()
   to execute.  argv[2] cannot be greater than MAX_CALLS.

   argv[3] is the (maximum) size of each memory chunk.

   If the NDEBUG macro is not defined, then initialize and check
   the contents of each memory chunk.

   At the end of the process, write the heap memory and CPU time
   consumed to stdout, and return 0. */

{
   int iTestNum = 0;
   int iCount = 0;
   int iSize = 0;
   clock_t iInitialClock;
   clock_t iFinalClock;
   char *pcInitialBreak;
   char *pcFinalBreak;
   int iMemoryConsumed;
   double dTimeConsumed;

   /* Get the command-line arguments. */
   getArgs(argc, argv, &iTestNum, &iCount, &iSize);

   /* Start printing the results. */
   printf("%16s %12s %7d %6d ", argv[0], argv[1], iCount, iSize);
   fflush(stdout);

   /* Save the initial clock and program break. */
   iInitialClock = clock();
   pcInitialBreak = sbrk(0);

   /* Set the process's CPU time limit. */
   setCpuLimit();

   /* Call the specified test function. */
   (*(apfTestFunction[iTestNum]))(iCount, iSize);

   /* Save the final clock and program break. */
   pcFinalBreak = sbrk(0);
   iFinalClock = clock();

   /* Use the initial and final clocks and program breaks to compute
      CPU time and heap memory consumed. */
   iMemoryConsumed = (int)(pcFinalBreak - pcInitialBreak);
   dTimeConsumed =
      ((double)(iFinalClock - iInitialClock)) / CLOCKS_PER_SEC;

   /* Finish printing the results. */
   printf("%6.2f %10d\n", dTimeConsumed, iMemoryConsumed);
   return 0;
}

/*--------------------------------------------------------------------*/

static void getArgs(int argc, char *argv[],
   int *piTestNum, int *piCount, int *piSize)

/* Get command-line arguments *piTestNum, *piCount, and *piSize,
   from argument vector argv.  argc is the number of used elements
   in argv.  Exit if any of the arguments is invalid.  */

{
   int i;
   int iTestCount;

   if (argc != 4)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   /* Get the test number. */
   iTestCount = (int)(sizeof(apcTestName) / sizeof(apcTestName[0]));
   for (i = 0; i < iTestCount; i++)
      if (strcmp(argv[1], apcTestName[i]) == 0)
      {
         *piTestNum = i;
         break;
      }
   if (i == iTestCount)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Valid testnames:\n");
      for (i = 0; i < iTestCount; i++)
         fprintf(stderr, " %s", apcTestName[i]);
      fprintf(stderr, "\n");
      exit(EXIT_FAILURE);
   }

   /* Get the count. */
   if (sscanf(argv[2], "%d", piCount) != 1)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Count must be numeric\n");
      exit(EXIT_FAILURE);
   }
   if (*piCount <= 0)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Count must be positive\n");
      exit(EXIT_FAILURE);
   }
   if (*piCount > MAX_CALLS)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Count cannot be greater than %d\n", MAX_CALLS);
      exit(EXIT_FAILURE);
   }

   /* Get the size. */
   if (sscanf(argv[3], "%d", piSize) != 1)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Size must be numeric\n");
      exit(EXIT_FAILURE);
   }
   if (*piSize <= 0)
   {
      fprintf(stderr, "Usage: %s testname count size\n", argv[0]);
      fprintf(stderr, "Size must be positive\n");
      exit(EXIT_FAILURE);
   }
}

/*--------------------------------------------------------------------*/

static void setCpuLimit(void)

/* Set the process's resource limit to 300 seconds (5 minutes).
   After 300 seconds, the OS will send a SIGKILL signal to the
   process. */

{
   struct rlimit sRlimit;
   sRlimit.rlim_cur = 300;
   sRlimit.rlim_max = 300;
   setrlimit(RLIMIT_CPU, &sRlimit);
}

/*--------------------------------------------------------------------*/

#define ASSURE(i) assure(i, __LINE__)

static void assure(int iSuccessful, int iLineNum)

/* If !iSuccessful, print an error message indicating that the test
   at line iLineNum failed. */

{
   if (! iSuccessful)
      fprintf(stderr, "Test at line %d failed.\n", iLineNum);
}

/*--------------------------------------------------------------------*/

static void testLifoFixed(int iCount, int iSize)

/* Allocate and free iCount memory chunks, each of size iSize, in
   last-in-first-out order. */

{
   int i;

   /* Call mymalloc() repeatedly to fill apcChunks. */
   for (i = 0; i < iCount; i++)
   {
      apcChunks[i] = (char*)mymalloc((size_t)iSize);
      ASSURE(apcChunks[i] != NULL);

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of iRand.
            So later, given iRand, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
   }

   /* Call myfree() repeatedly to free the chunks in
      LIFO order. */
   for (i = iCount - 1; i >= 0; i--)
   {
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif

      myfree(apcChunks[i]);
   }
}

/*--------------------------------------------------------------------*/

static void testFifoFixed(int iCount, int iSize)

/* Allocate and free iCount memory chunks, each of size iSize, in
   first-in-first-out order. */

{
   int i;

   /* Call mymalloc() repeatedly to fill apcChunks. */
   for (i = 0; i < iCount; i++)
   {
      apcChunks[i] = (char*)mymalloc((size_t)iSize);
      ASSURE(apcChunks[i] != NULL);

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of iRand.
            So later, given iRand, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
   }

   /* Call myfree() repeatedly to free the chunks in
      FIFO order. */
   for (i = 0; i < iCount; i++)
   {
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif

      myfree(apcChunks[i]);
   }
}

/*--------------------------------------------------------------------*/

static void testLifoRandom(int iCount, int iSize)

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in last-in-first-out order. */

{
   int i;

   /* Fill aiSizes, an array of random integers in the range 1 to
      iSize. */
   for (i = 0; i < iCount; i++)
      aiSizes[i] = (rand() % iSize) + 1;

   /* Call mymalloc() repeatedly to fill apcChunks. */
   for (i = 0; i < iCount; i++)
   {
      apcChunks[i] = (char*)mymalloc((size_t)aiSizes[i]);
      ASSURE(apcChunks[i] != NULL);

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of iRand.
            So later, given iRand, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < aiSizes[i]; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
   }

   /* Call myfree() repeatedly to free the chunks in
      LIFO order. */
   for (i = iCount - 1; i >= 0; i--)
   {
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < aiSizes[i]; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif

      myfree(apcChunks[i]);
   }
}

/*--------------------------------------------------------------------*/

static void testFifoRandom(int iCount, int iSize)

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in first-in-first-out order. */

{
   int i;

   /* Fill aiSizes, an array of random integers in the range 1 to
      iSize. */
   for (i = 0; i < iCount; i++)
      aiSizes[i] = (rand() % iSize) + 1;

   /* Call mymalloc() repeatedly to fill apcChunks. */
   for (i = 0; i < iCount; i++)
   {
      apcChunks[i] = (char*)mymalloc((size_t)aiSizes[i]);
      ASSURE(apcChunks[i] != NULL);

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of iRand.
            So later, given iRand, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < aiSizes[i]; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
   }

   /* Call myfree() repeatedly to free the chunks in
      FIFO order. */
   for (i = 0; i < iCount; i++)
   {
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < aiSizes[i]; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif

      myfree(apcChunks[i]);
   }
}

/*--------------------------------------------------------------------*/

static void testRandomFixed(int iCount, int iSize)

/* Allocate and free iCount memory chunks, each of size iSize, in
   a random order. */

{
   int i;
   int iRand;
   int iLogicalArraySize;

   iLogicalArraySize = (iCount / 3) + 1;

   /* Call mymalloc() and myfree() in a randomly
      interleaved manner. */
   iRand = 0;
   for (i = 0; i < iCount; i++)
   {
      apcChunks[iRand] = (char*)mymalloc((size_t)iSize);
      ASSURE(apcChunks[iRand] != NULL);

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of iRand.
            So later, given iRand, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((iRand % 10) + '0');
         for (iCol = 0; iCol < iSize; iCol++)
            apcChunks[iRand][iCol] = c;
      }
      #endif

      /* Assign some random integer to iRand. */
      iRand = rand() % iLogicalArraySize;

      /* If apcChunks[iRand] contains a chunk, free it and set
         apcChunks[iRand] to NULL. */
      if (apcChunks[iRand] != NULL)
      {
         #ifndef NDEBUG
         {
            /* Check the chunk that is about to be freed to make sure
               that its contents haven't been corrupted. */
            int iCol;
            char c = (char)((iRand % 10) + '0');
            for (iCol = 0; iCol < iSize; iCol++)
               ASSURE(apcChunks[iRand][iCol] == c);
         }
         #endif

         myfree(apcChunks[iRand]);
         apcChunks[iRand] = NULL;
      }
   }

   /* Free the rest of the chunks. */
   for (i = 0; i < iLogicalArraySize; i++)
   {
      if (apcChunks[i] != NULL)
      {
         #ifndef NDEBUG
         {
            /* Check the chunk that is about to be freed to make sure
               that its contents haven't been corrupted. */
            int iCol;
            char c = (char)((i % 10) + '0');
            for (iCol = 0; iCol < iSize; iCol++)
               ASSURE(apcChunks[i][iCol] == c);
         }
         #endif

         myfree(apcChunks[i]);
         apcChunks[i] = NULL;
      }
   }
}

/*--------------------------------------------------------------------*/

static void testRandomRandom(int iCount, int iSize)

/* Allocate and free iCount memory chunks, each of some random size
   less than iSize, in a random order. */

{
   int i;
   int iRand;
   int iLogicalArraySize;

   iLogicalArraySize = (iCount / 3) + 1;

   /* Fill aiSizes, an array of random integers in the range 1
      to iSize. */
   for (i = 0; i < iLogicalArraySize; i++)
      aiSizes[i] = (rand() % iSize) + 1;

   /* Call mymalloc() and myfree() in a randomly
      interleaved manner. */
   iRand = 0;
   for (i = 0; i < iCount; i++)
   {
      apcChunks[iRand] = (char*)mymalloc((size_t)aiSizes[iRand]);
      ASSURE(apcChunks[iRand] != NULL);

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of iRand.
            So later, given iRand, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
         char c = (char)((iRand % 10) + '0');
         for (iCol = 0; iCol < aiSizes[iRand]; iCol++)
            apcChunks[iRand][iCol] = c;
      }
      #endif

      /* Assign some random integer to iRand. */
      iRand = rand() % iLogicalArraySize;

      /* If apcChunks[iRand] contains a chunk, free it and set
         apcChunks[iRand] to NULL. */
      if (apcChunks[iRand] != NULL)
      {
         #ifndef NDEBUG
         {
            /* Check the chunk that is about to be freed to make sure
               that its contents haven't been corrupted. */
            int iCol;
            char c = (char)((iRand % 10) + '0');
            for (iCol = 0; iCol < aiSizes[iRand]; iCol++)
			{
               if (apcChunks[iRand][iCol] != c)
			   {
				   printf("%p weird!\n", &apcChunks[iRand][iCol]);
				   exit(0);
			   }

               ASSURE(apcChunks[iRand][iCol] == c);
			}
         }
         #endif

         myfree(apcChunks[iRand]);
         apcChunks[iRand] = NULL;
      }
   }

   /* Free the rest of the chunks. */
   for (i = 0; i < iLogicalArraySize; i++)
   {
      if (apcChunks[i] != NULL)
      {
         #ifndef NDEBUG
         {
            /* Check the chunk that is about to be freed to make sure
               that its contents haven't been corrupted. */
            int iCol;
            char c = (char)((i % 10) + '0');
            for (iCol = 0; iCol < aiSizes[i]; iCol++)
               ASSURE(apcChunks[i][iCol] == c);
         }
         #endif

         myfree(apcChunks[i]);
         apcChunks[i] = NULL;
      }
   }
}

/*--------------------------------------------------------------------*/

static void testWorst(int iCount, int iSize)

/* Allocate and free iCount memory chunks, each of some size less
   than iSize, in the worst possible order for a HeapMgr that is
   implemented using a single linked list. */

{
   int i;

   /* Fill the array with chunks of increasing size, each separated by
      a small dummy chunk. */
   i = 0;
   while (i < iCount)
   {
      apcChunks[i] = mymalloc((size_t)((i * iSize / iCount) + 1));
      ASSURE((i == 0) || (apcChunks[i] != NULL));

      #ifndef NDEBUG
      {
         /* Fill the newly allocated chunk with some character.
            The character is derived from the last digit of iRand.
            So later, given iRand, we can check to make sure that
            the contents haven't been corrupted. */
         int iCol;
	 int max = (i * iSize / iCount) + 1;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < max; iCol++)
            apcChunks[i][iCol] = c;
      }
      #endif
      i++;
      apcChunks[i] = mymalloc((size_t)1);
      i++;
   }

   /* Free the non-dummy chunks in reverse order.  Thus a HeapMgr
      implementation that uses a single linked list will be in a
      worst-case state:  the list will contain chunks in increasing
      order by size. */
   i = iCount;
   while (i >= 2)
   {
      i--;
      i--;
      #ifndef NDEBUG
      {
         /* Check the chunk that is about to be freed to make sure
            that its contents haven't been corrupted. */
         int iCol;
	 int max = (i * iSize / iCount) + 1;
         char c = (char)((i % 10) + '0');
         for (iCol = 0; iCol < max; iCol++)
            ASSURE(apcChunks[i][iCol] == c);
      }
      #endif
      myfree(apcChunks[i]);
   }

   /* Allocate chunks in decreasing order by size, thus maximizing the
      amount of list traversal required. */
   i = iCount;
   while (i >= 2)
   {
      i--;
      i--;
      apcChunks[i] = mymalloc((size_t)((i * iSize / iCount) + 1));
      ASSURE(apcChunks != NULL);
   }

   /* Free all chunks. */
   for (i = 0; i < iCount; i++)
      myfree(apcChunks[i]);
}
