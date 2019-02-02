#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* for skeleton code */
#include <unistd.h> /* for getopt */
#include "str.h"

#define FIND_STR    "-f"
#define REPLACE_STR "-r"
#define DIFF_STR    "-d"

#define MAX_STR_LEN 1023

#define FALSE 0
#define TRUE  1

typedef enum {
   INVALID,
   FIND,
   REPLACE,
   DIFF
} CommandType;

size_t StrGetLength(const char* pcSrc)
{
  const char *pcEnd;
  assert(pcSrc); /* NULL address, 0, and FALSE are identical. */
  pcEnd = pcSrc;

  while (*pcEnd) /* null character and FALSE are identical. */
    pcEnd++;

  return (size_t)(pcEnd - pcSrc);
}

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


/*--------------------------------------------------------------------*/
/* PrintUsage()
   print out the usage of the Simple Grep Program                     */
/*--------------------------------------------------------------------*/
void
PrintUsage(const char* argv0)
{
   const static char *fmt =
      "Simple Grep (sgrep) Usage:\n"
      "%s [COMMAND] [OPTIONS]...\n"
      "\nCOMMNAD\n"
      "\tFind: -f [search-string\n"
      "\tReplace: -r [word1] [word2]\n"
      "\tDiff: -d [file1] [file2]\n";

   printf(fmt, argv0);
}
/*-------------------------------------------------------------------*/
/* DoFind()
   Your task:
   1. Do argument validation
   - String or file argument length is no more than 1023
   - If you encounter a command-line argument that's too long,
   print out "Error: argument is too long"

   2. Read the each line from standard input (stdin)
   - If you encounter a line larger than 1023 bytes,
   print out "Error: input line is too long"
   - Error message should be printed out to standard error (stderr)

   3. Check & print out the line contains a given string (word1)

   Tips:
   - fgets() is an useful function to read characters from file. Note
   that the fget() reads until newline or the end-of-file is reached.
   - fprintf(sderr, ...) should be useful for printing out error
   message to standard error

   NOTE: If there is any problem, return FALSE; if not, return TRUE  */
/*-------------------------------------------------------------------*/
int
DoFind(const char *pcSearch)
{
    char buf[MAX_STR_LEN + 2];
    int len;

    if ((len = StrGetLength(pcSearch)) > MAX_STR_LEN) {
        fprintf(stderr, "Error: argument is too long\n");
        return FALSE;
    }

    while (fgets(buf, sizeof(buf), stdin)) {

        if ((len = StrGetLength(buf)) > MAX_STR_LEN) {
            fprintf(stderr, "Error: input line is too long\n");
            return FALSE;
        }

        if (StrSearch(buf, pcSearch) != NULL) {
            fprintf(stdout, "%s", buf);
        }
    }

    return TRUE;
}
/*-------------------------------------------------------------------*/
/* DoReplace()
   Your task:
   1. Do argument validation
      - String length is no more than 1023
      - If you encounter a command-line argument that's too long,
        print out "Error: argument is too long"
      - word1 cannot be an empty string

   2. Read the each line from standard input (stdin)
      - If you encounter a line larger than 1023 bytes,
        print out "Error: input line is too long"
      - Error message should be printed out to standard error (stderr)

   3. Replace the string and print out the replaced string

   NOTE: If there is any problem, return FALSE; if not, return TRUE  */
/*-------------------------------------------------------------------*/
int
DoReplace(const char *pcWord1, const char *pcWord2)
{
    char buf[MAX_STR_LEN + 2];
    char Dest[MAX_STR_LEN + 2];
    int len;

    if ((len = StrGetLength(pcWord1)) > MAX_STR_LEN
            || ((len = StrGetLength(pcWord2)) > MAX_STR_LEN)) {
        fprintf(stderr, "Error: argument is too long\n");
        return FALSE;
    }

    if ((pcWord1 != NULL) && (*pcWord1 == '\0')) {
        fprintf(stderr, "Error: Can't replace an empty substring\n");
        return FALSE;
    }

    while (fgets(buf, sizeof(buf), stdin)) {

        if ((len = StrGetLength(buf)) > MAX_STR_LEN) {
            fprintf(stderr, "Error: input line is too long\n");
            return FALSE;
        }

        StrReplace(Dest, buf, pcWord1, pcWord2);

        fprintf(stdout, "%s", Dest);
    }

    return TRUE;
}
/*-------------------------------------------------------------------*/
/* DoDiff()
   Your task:
   1. Do argument validation
     - file name length is no more than 1023
     - If a command-line argument is too long,
       print out "Error: arugment is too long" to stderr

   2. Open the two files
      - The name of files are given by two parameters
      - If you fail to open either file, print out error messsage
      - Error message: "Error: failed to open file [filename]\n"
      - Error message should be printed out to stderr

   3. Read the each line from each file
      - If you encounter a line larger than 1023 bytes,
        print out "Error: input line [filename] is too long"
      - Error message should be printed out to stderr

   4. Compare the two files (file1, file2) line by line

   5. Print out any different line with the following format
      file1@linenumber:file1's line
      file2@linenumber:file2's line

   6. If one of the files ends earlier than the other, print out an
      error message "Error: [filename] ends early at line XX", where
      XX is the final line number of [filename].

   NOTE: If there is any problem, return FALSE; if not, return TRUE  */
/*-------------------------------------------------------------------*/
int
DoDiff(const char *file1, const char *file2)
{
    char buf1[MAX_STR_LEN + 2];
    char buf2[MAX_STR_LEN + 2];
    char *ret1, *ret2;
    int len, count = 1;
    FILE *f1;
    FILE *f2;

    if ((len = StrGetLength(file1)) > MAX_STR_LEN
            || ((len = StrGetLength(file2)) > MAX_STR_LEN)) {
        fprintf(stderr, "Error: argument is too long\n");
        return FALSE;
    }

    f1 = fopen(file1, "r");
    f2 = fopen(file2, "r");

    if (f1 == NULL) {
        fprintf(stderr, "Error: failed to open file [%s]\n", file1);
        if (f2 != NULL) fclose(f2);
        return FALSE;
    }
    if (f2 == NULL) {
        fprintf(stderr, "Error: failed to open file [%s]\n", file2);
        fclose(f1);
        return FALSE;
    }

    while (TRUE) {
        ret1 = fgets(buf1, sizeof(buf1), f1);
        ret2 = fgets(buf2, sizeof(buf2), f2);

        if (ret1 == NULL && ret2 == NULL) break;

        if (ret1 == NULL && ret2 != NULL) {
            fprintf(stderr, "Error: [%s] ends early at line %d\n", file1, count-1);
            fclose(f1);  fclose(f2);
            return FALSE;
        }

        if (ret2 == NULL && ret1 != NULL) {
            fprintf(stderr, "Error: [%s] ends early at line %d\n", file2, count-1);
            fclose(f1);  fclose(f2);
            return FALSE;
        }

        if ((len = StrGetLength(buf1)) > MAX_STR_LEN) {
            fprintf(stderr, "Error: input line [%s] is too long\n", file1);
            fclose(f1);  fclose(f2);
            return FALSE;
        }

        if ((len = StrGetLength(buf2)) > MAX_STR_LEN) {
            fprintf(stderr, "Error: input line [%s] is too long\n", file2);
            return FALSE;
        }

        if (StrCompare(buf1, buf2) != 0) {
            fprintf(stdout, "%s@%d:%s", file1, count, buf1);
            fprintf(stdout, "%s@%d:%s", file2, count, buf2);
        }

        count++;
    }

    fclose(f1);
    fclose(f2);

    return TRUE;
}
/*-------------------------------------------------------------------*/
/* CommandCheck()
   - Parse the command and check number of argument.
   - It returns the command type number
   - This function only checks number of argument.
   - If the unknown function is given or the number of argument is
   different from required number, this function returns FALSE.

   Note: You SHOULD check the argument rule later                    */
/*-------------------------------------------------------------------*/
int
CommandCheck(const int argc, const char *argv1)
{
   int cmdtype = INVALID;

   /* check minimum number of argument */
   if (argc < 3)
      return cmdtype;

   /* check command type */
   if (strcmp(argv1, FIND_STR) == 0) {
      if (argc != 3)
         return FALSE;
      cmdtype = FIND;
   }
   else if (strcmp(argv1, REPLACE_STR) == 0) {
      if (argc != 4)
         return FALSE;
      cmdtype = REPLACE;
   }
   else if (strcmp(argv1, DIFF_STR) == 0) {
      if (argc != 4)
         return FALSE;
      cmdtype = DIFF;
   }

   return cmdtype;
}
/*-------------------------------------------------------------------*/
int
main(const int argc, const char *argv[])
{
   int type, ret;

   /* Do argument check and parsing */
   if (!(type = CommandCheck(argc, argv[1]))) {
      fprintf(stderr, "Error: argument parsing error\n");
      PrintUsage(argv[0]);
      return (EXIT_FAILURE);
   }

   /* Do appropriate job */
   switch (type) {
      case FIND:
         ret = DoFind(argv[2]);
         break;
      case REPLACE:
         ret = DoReplace(argv[2], argv[3]);
         break;
      case DIFF:
         ret = DoDiff(argv[2], argv[3]);
         break;
   }

   return (ret)? EXIT_SUCCESS : EXIT_FAILURE;
}
