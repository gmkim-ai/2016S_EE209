#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"

enum { FALSE = 0, TRUE } pipe_exist = FALSE;
enum TokenType { TOKEN_ORD=0, TOKEN_EXT };   //ordinary and extraordinary

struct Token {
     enum TokenType type;
     char *str;
};


typedef struct Token * Token_T;

struct DynArray
{
	/* The number of elements in the DynArray from the client's
	   point of view. */
	int iLength;

	/* The number of elements in the array that underlies the
	   DynArray. */
	int iPhysLength;

	/* The array that underlies the DynArray. */
	const void **ppvArray;
};

DynArray_T DynArray_new(int iLength)
{
	DynArray_T oDynArray;

	assert(iLength >= 0);

	oDynArray = (struct DynArray*)malloc(sizeof(struct DynArray));
	if (oDynArray == NULL)
		return NULL;

	oDynArray->iLength = iLength;
	if (iLength > 2)
		oDynArray->iPhysLength = iLength;
	else
		oDynArray->iPhysLength = 2;

	oDynArray->ppvArray =
		(const void**)calloc((size_t)oDynArray->iPhysLength,
							 sizeof(void*));
	if (oDynArray->ppvArray == NULL) {
		free(oDynArray);
		return NULL;
	}

	return oDynArray;
}
static int DynArray_grow(DynArray_T oDynArray)
{
	int iNewLength;
	const void **ppvNewArray;

	assert(oDynArray != NULL);

	iNewLength = oDynArray->iPhysLength * 2;

	ppvNewArray = (const void**)
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
int DynArray_add(DynArray_T oDynArray, const void *pvElement)
{
	assert(oDynArray != NULL);

	if (oDynArray->iLength == oDynArray->iPhysLength)
		if (!DynArray_grow(oDynArray))
			return 0;

	oDynArray->ppvArray[oDynArray->iLength] = pvElement;
	oDynArray->iLength++;


	return 1;
}

int lexical( DynArray_T TokenArray, char *Line ) {
    assert(Line != NULL);

    if (Line[0] == '\0') {
        return TRUE;
    }

    Token_T token;
    char *start, *string;
    int i, j, k, count, length;
    enum DFAState {normal, malloc, quote};
    enum DFAState state = normal;
    enum DFAState backstate = malloc;

    if (isspace(Line[0]) || Line[0] == '|') state = malloc;
    if (Line[0] == '\"') state = quote;
    length = strlen(Line);

    for (i=0; i<length; i++) {
        switch(state) {

            case normal:
                if (backstate == malloc) {
                    start = &Line[i];
                    count = 0;
                }
                if (Line[i+1] == '\"') state = quote;
                else if (isspace(Line[i+1]) || Line[i+1] == '|') state = malloc;

                count++;
                backstate = normal;
                break;

            case quote:
                if (backstate == malloc) {
                    start = &Line[i];
                    count = 0;
                }
                if (Line[i+1] == '\"') state = normal;

                count++;
                backstate = quote;
                break;

            case malloc:
                if (backstate == malloc && isspace(Line[i])) {
                    state = normal;
                    if (Line[i+1] == '\"') state = quote;
                    else if (isspace(Line[i+1]) || Line[i+1] == '|' || Line[i+1] == NULL) state = malloc;
                    continue;
                }
                if (backstate == malloc && Line[i] == '|') {
                    token = (Token_T) calloc(1, sizeof(struct Token));
                    if (token == NULL) {
                        fprintf(stderr, "./mani_ish: fail to allocate\n");
                        return FALSE;
                    }
                    string = (char *)calloc(2,1);
                    if (string == NULL) {
                        fprintf(stderr, "./mani_ish: fail to allocate\n");
                        free(token);
                        return FALSE;
                    }
                    token->str = string;
                    string[0] = '|';
                    token->type = TOKEN_EXT;
                    DynArray_add(TokenArray, token);
                    state = normal;
                    if (Line[i+1] == '\"') state = quote;
                    else if (isspace(Line[i+1]) || Line[i+1] == '|' || Line[i+1] == NULL) state = malloc;
                    continue;
                }
                token = (Token_T) calloc(1, sizeof(struct Token));
                if (token == NULL) {
                    fprintf(stderr, "./mani_ish: fail to allocate\n");
                    return FALSE;
                }
                string = (char *)calloc(count+1,1);
                if (string == NULL) {
                    fprintf(stderr, "./mani_ish: fail to allocate\n");
                    free(token);
                    return FALSE;
                }
                token->str = string;
                k = 0;
                for (j=0; j<count; j++) {
                    if (*(start+j) != '\"') {
                        string[k] = *(start+j);
                        k++;
                    }
                }
                DynArray_add(TokenArray, token);

                if (Line[i] == '|') {
                    token = (Token_T) calloc(1, sizeof(struct Token));
                    if (token == NULL) {
                        fprintf(stderr, "./mani_ish: fail to allocate\n");
                        return FALSE;
                    }
                    string = (char *)calloc(2,1);
                    if (string == NULL) {
                        fprintf(stderr, "./mani_ish: fail to allocate\n");
                        free(token);
                        return FALSE;
                    }
                    token->str = string;
                    string[0] = '|';
                    token->type = TOKEN_EXT;
                    DynArray_add(TokenArray, token);
                }

                state = normal;
                if (Line[i+1] == '\"') state = quote;
                else if (isspace(Line[i+1]) || Line[i+1] == '|' || Line[i+1] == NULL) state = malloc;

                backstate = malloc;
                break;
        }
    }
    if (state == quote) {
        fprintf(stderr, "./mani_ish: unmatched quote\n");
        return FALSE;
    }
    if (state == normal) {
        token = (Token_T) calloc(1, sizeof(struct Token));
        if (token == NULL) {
            fprintf(stderr, "./mani_ish: fail to allocate\n");
            return FALSE;
        }
        string = (char *)calloc(count+1,1);
        if (string == NULL) {
            fprintf(stderr, "./mani_ish: fail to allocate\n");
            free(token);
            return FALSE;
        }
        token->str = string;
        if (token == NULL) {
            fprintf(stderr, "./mani_ish: fail to allocate\n");
            return FALSE;
        }
        k = 0;
        for (j=0; j<count; j++) {
            if (*(start+j) != '\"') {
                string[k] = *(start+j);
                k++;
            }
        }
        DynArray_add(TokenArray, token);
    }
    return TRUE;
}

void DynArray_free(DynArray_T oDynArray)
{
	assert(oDynArray != NULL);

	free(oDynArray->ppvArray);
	free(oDynArray);
}

int main() {
    char arr[50];
    int i;
    DynArray_T oDynArray;

    while(1) {
        oDynArray = DynArray_new(0);
        gets(arr);
        lexical(oDynArray, arr);
        for (i=0; i < oDynArray->iLength; i++) {
            Token_T p = oDynArray->ppvArray[i];
            printf("%s's type is",p->str);
            if (p->type == TOKEN_EXT) {
                printf("TOKEN_EXT\n");
            }
            else if (p->type == TOKEN_ORD) {
                printf("TOKEN_ORD\n");
            }
        }
        DynArray_free(oDynArray);
    }
    return 0;
}

