#include <assert.h> /* to use assert() */
#include <stdio.h>
#include <string.h>
#include "str.h"

/* Your task is:
   1. Rewrite the body of "Part 1" functions - remove the current
      body that simply calls the corresponding C standard library
      function.
   2. Fill out the body of "Part 2" functions
   3. Write appropriate comment per each function
*/

/* Part 1 */
/*------------------------------------------------------------------------*/
size_t StrGetLength(const char* pcSrc)
{
  const char *pcEnd;
  assert(pcSrc); /* NULL address, 0, and FALSE are identical. */
  pcEnd = pcSrc;

  while (*pcEnd) /* null character and FALSE are identical. */
    pcEnd++;

  return (size_t)(pcEnd - pcSrc);
}

/*------------------------------------------------------------------------*/
char *StrCopy(char *pcDest, const char* pcSrc)
{
    assert(pcSrc);
    char *start = pcDest;

    *pcDest = *pcSrc;
    while( *pcSrc != '\0') {
        pcDest++;
        pcSrc++;
        *pcDest = *pcSrc;
    }

    return start;
}

/*------------------------------------------------------------------------*/
int StrCompare(const char* pcS1, const char* pcS2)
{
    assert(pcS1);
    assert(pcS2);
    while(*pcS1 != '\0' || *pcS2 != '\0') {
        if (*pcS1 > *pcS2) return 1;
        else if (*pcS1 < *pcS2) return -1;
        pcS1++;
        pcS2++;
    }

    return 0;
}
/*------------------------------------------------------------------------*/
char *StrSearch(const char* pcHaystack, const char *pcNeedle)
{
    assert(pcHaystack);
    assert(pcNeedle);
    char *Letter = (char *)pcNeedle;

    if (*pcNeedle == '\0') {
        char *start = (char *)pcHaystack;
        return start;
    }


    while (*pcHaystack != '\0') {
        if (*pcHaystack == *Letter) {
            char *start = (char *)pcHaystack;

            while (1) {
                Letter++;
                pcHaystack++;
                if (*Letter == '\0') return start;
                else if (*pcHaystack == '\0') return NULL;
                else if (*pcHaystack != *Letter) {
                    Letter = (char *)pcNeedle;
                    break;
                }
            }
        }

        pcHaystack++;
    }

    return NULL;
}
/*------------------------------------------------------------------------*/
char *StrConcat(char *pcDest, const char* pcSrc)
{
    assert(pcSrc);
    char *start = pcDest;

    while (*pcDest != '\0') {
        pcDest++;
    }

    while (*pcSrc != '\0') {
        *pcDest = *pcSrc;
        pcDest++;
        pcSrc++;
    }

    *pcDest = '\0';

    return start;
}
int StrReplace(char *pcDest, const char *pcSrc,
                  const char *pcF, const char *pcR)
{
    int count = 0;
    assert(pcSrc);
    assert(pcF);
    assert(pcR);
    if ((pcF != NULL) && (*pcF == '\0')) return -1;
    while (*pcSrc != '\0') {
        if (pcSrc == StrSearch(pcSrc, pcF)) {
            StrCopy(pcDest, pcR);
            pcSrc = pcSrc + StrGetLength(pcF);
            pcDest = pcDest + StrGetLength(pcR);
            count++;
        }
        else {
            *pcDest = *pcSrc;
            pcDest++;
            pcSrc++;
        }
    }
    *pcDest = *pcSrc;

    return count;
}
char *StrJoin(char *pcDest, const char *pcSrcs[],
              const int n, const char *pcSep)
{
    int i, count = 0;
    for (i = 0; i < n; i++) {
        assert(pcSrcs[i]);
    }

    printf("%d", count);

    if (n <= 0) return NULL;

    char *start = pcDest;

    for (i = 0; i < n; i++) {
        StrCopy(pcDest, pcSrcs[i]);
        pcDest = pcDest + StrGetLength(pcSrcs[i]);
        if (i == n-1) break;
        StrCopy(pcDest, pcSep);
        pcDest = pcDest + StrGetLength(pcSep);
    }

    return start;
}
int main()
{
    char *srcs[5] = {"Apple", "Samsung", "Xiaomi", "Huawei", "Google"};
    char dest[1000];
    char *ret;

    ret = StrJoin(dest, srcs, 5, " vs. ");
    printf("Result: %s\n", dest);
}
