/*--------------------------------------------------------------------*/
/* mani_lexical.c                                                     */
/* Author: Gyeongman Kim  20150073                                    */
/*--------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#include "mani_ish.h"

/*--------------------------------------------------------------------*/
/* lexical step divide word by white-space word with DFA. there is
    malloc, normal, quote state and we have to allocate memory
    and save word in Token structure when normal state go to malloc
    state or quote state go to malloc state. And we should check
    final state because when final state is normal, we should add Token
    structure in dynamic array and when final state is quote state,
    we should print error message. Return TRUE if success for divide
    or return FLASE if there are any error.*/
int lexical( DynArray_T TokenArray, char *Line, char *argv[]) {
    assert(Line != NULL);
    assert(TokenArray != NULL);
    assert(TokenArray->ppvArray != NULL);
    assert(TokenArray->iLength >= 0);

    Token_T token;
    char *start, *string;
    int i, j, k, count, length;
    enum DFAState {normal, malloc, quote};
    enum DFAState state = normal;
    enum DFAState backstate = malloc;
    char quote_kind;

    /*choose first state before start for() */
    if (isspace(Line[0]) || Line[0] == '|' || Line[0] == '<'
                            || Line[0] == '>') state = malloc;
    if (Line[0] == '\"' || Line[0] == '\'') state = quote;
    length = strlen(Line);

    for (i=0; i<length; i++) {
        switch(state) {

            case normal:
                if (backstate == malloc) {   //word start
                    start = &Line[i];
                    count = 0;
                }
                if (Line[i+1] == '\"'||Line[i+1] == '\'')state = quote;
                else if (isspace(Line[i+1]) || Line[i+1] == '|'
                    || Line[i+1] =='<'||Line[i+1]== '>')state = malloc;

                count++;              //count
                backstate = normal;
                break;

            case quote:
                if (backstate == malloc) {  //word start
                    quote_kind = Line[i];
                    start = &Line[i];
                    count = 0;
                }
                else if (backstate == normal) {  //save quote info
                    quote_kind = Line[i];
                }
                if (Line[i+1] == quote_kind) state = normal;

                count++;         //count
                backstate = quote;
                break;

            case malloc:
                if (backstate == malloc && isspace(Line[i])) {
                    state = normal;       //do nothing and change state
                    if (Line[i+1] == '\"'
                            || Line[i+1] == '\'') state = quote;
                    else if (isspace(Line[i+1]) || Line[i+1] == '|'
                            || Line[i+1] == '\0'  || Line[i+1] == '<'
                                    || Line[i+1] == '>')state = malloc;
                    continue;
                }
                /*when special char like |, <, > */
                if (backstate == malloc && (Line[i] == '|'
                            || Line[i] == '<' || Line[i] == '>')) {
                    token = (Token_T) calloc(1, sizeof(struct Token));
                    if (token == NULL) {
                        fprintf(stderr, "%s: fail to allocate\n"
                                                        , argv[0]);
                        return FALSE;
                    }
                    string = (char *)calloc(2,1);
                    if (string == NULL) {
                        fprintf(stderr, "%s: fail to allocate\n"
                                                        , argv[0]);
                        free(token);
                        return FALSE;
                    }
                    token->str = string;    //save Token structure info
                    string[0] = Line[i];
                    token->type = TOKEN_EXT;
                    DynArray_add(TokenArray, token);  //add word
                    state = normal;
                    if (Line[i+1] == '\"') state = quote;
                    else if (isspace(Line[i+1]) || Line[i+1] == '|'
                            || Line[i+1] == '\0' || Line[i+1] == '<'
                                    || Line[i+1] == '>')state= malloc;
                    continue;
                }
                /*if backstate is not malloc state*/
                token = (Token_T) calloc(1, sizeof(struct Token));
                if (token == NULL) {
                    fprintf(stderr, "%s: fail to allocate\n", argv[0]);
                    return FALSE;
                }
                string = (char *)calloc(count+1,1);
                if (string == NULL) {
                    fprintf(stderr, "%s: fail to allocate\n", argv[0]);
                    free(token);
                    return FALSE;
                }
                token->str = string;
                k = 0;
                for (j=0; j<count; j++) {  //make word without quote
                    if (*(start+j) != quote_kind) {
                        string[k] = *(start+j);
                        k++;
                    }
                }
                DynArray_add(TokenArray, token);

                /*when special char like |, <, > */
                if (Line[i] == '|' || Line[i] == '<' || Line[i] == '>'){
                    token = (Token_T) calloc(1, sizeof(struct Token));
                    if (token == NULL) {
                        fprintf(stderr, "%s: fail to allocate\n"
                                                        , argv[0]);
                        return FALSE;
                    }
                    string = (char *)calloc(2,1);
                    if (string == NULL) {
                        fprintf(stderr, "%s: fail to allocate\n"
                                                        , argv[0]);
                        free(token);
                        return FALSE;
                    }
                    token->str = string;
                    string[0] = Line[i];
                    token->type = TOKEN_EXT; //change type to TOKEN_EXT
                    DynArray_add(TokenArray, token);
                }

                state = normal;
                if (Line[i+1] == '\"'
                            || Line[i+1] == '\'') state = quote;
                else if (isspace(Line[i+1]) || Line[i+1] == '|'
                        || Line[i+1] == '\0' || Line[i+1] == '<'
                                || Line[i+1] == '>') state = malloc;

                backstate = malloc;
                break;
        }
    }
    /*when finish for() */
    if (state == quote) {  //unmatched
        fprintf(stderr, "%s: unmatched quote\n", argv[0]);
        return FALSE;
    }
    if (state == normal) {  //need add to dynamic array
        token = (Token_T) calloc(1, sizeof(struct Token));
        if (token == NULL) {
            fprintf(stderr, "%s: fail to allocate\n", argv[0]);
            return FALSE;
        }
        string = (char *)calloc(count+1,1);
        if (string == NULL) {
            fprintf(stderr, "%s: fail to allocate\n", argv[0]);
            free(token);
            return FALSE;
        }
        token->str = string;
        k = 0;
        for (j=0; j<count; j++) {
            if (*(start+j) != quote_kind) {
                string[k] = *(start+j);
                k++;
            }
        }
        DynArray_add(TokenArray, token);   //add
    }
    return TRUE;
}
