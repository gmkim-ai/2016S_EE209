/*--------------------------------------------------------------------*/
/* mani_syntactic.c                                                   */
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



enum exist pipe_exist, redi_exist;

/*--------------------------------------------------------------------*/
/* syntactic step judge that this line is valid or unvalid. So, when
    command is built-in command, we know how many argument needed. So,
    judge this is valid command with number of argument. And check that
    there are many pipe or redirection. If there is no built-in command,
    then we will use execvp function, so just check that there are any
    pipe in first or last index of array, or there are any pipe that
    adjoin each other. Return TRUE when line is valid, and return FALSE
    when line is invalid.*/
int syntactic( DynArray_T TokenArray, char *argv[]) {
    assert(TokenArray != NULL);
    assert(TokenArray->ppvArray != NULL);
    assert(TokenArray->iLength >= 0);
    enum DFAState {normal, redil, redir, pipe};
    enum DFAState state = normal;
    enum DFAState backstate = normal;
    int i;
    char *builtin1 = "setenv";
    char *builtin2 = "unsetenv";
    char *builtin3 = "cd";
    char *builtin4 = "exit";

    pipe_exist = FALSE;  //First, assume that there are no pipe in line
    redi_exist = FALSE;  //First, assume no redirection in line
    if (TokenArray->ppvArray[0] == NULL) {
	return FALSE;
    }

    /*setenv function need 1 or 2 argument and there are no pipe. */
    if (!strcmp(((Token_T)(TokenArray->ppvArray[0]))->str, builtin1)) {
        if (TokenArray->iLength > 3 || TokenArray->iLength < 2) {
            fprintf(stderr, "%s: setenv takes two parameters\n"
                                                        , argv[0]);
            return FALSE;
        }
        if (((Token_T)(TokenArray->ppvArray[1]))->type == TOKEN_EXT) {
            fprintf(stderr,
                "%s: Pipe or redirection destination not specified\n"
                                                            , argv[0]);
            return FALSE;
        }
        if (TokenArray->iLength == 3) {  //argument number can be 1
            if (((Token_T)(TokenArray->ppvArray[2]))->type
                                                == TOKEN_EXT) {
                fprintf(stderr,
                "%s: Pipe or redirection destination not specified\n"
                                                            , argv[0]);
                return FALSE;
            }
        }
        return TRUE;
    }
    /*unsetenv function need 1 argument and there are no pipe. */
    if (!strcmp(((Token_T)(TokenArray->ppvArray[0]))->str, builtin2)) {
        if (TokenArray->iLength > 2 || TokenArray->iLength < 2) {
            fprintf(stderr, "%s: unsetenv takes one parameter\n"
                                                        , argv[0]);
            return FALSE;
        }
        if (((Token_T)(TokenArray->ppvArray[1]))->type == TOKEN_EXT) {
            fprintf(stderr,
                "%s: Pipe or redirection destination not specified\n"
                                                            , argv[0]);
            return FALSE;
        }
        return TRUE;
    }
    /*cd function need 0 or 1 argument and there are no pipe. */
    if (!strcmp(((Token_T)(TokenArray->ppvArray[0]))->str, builtin3)) {
        if (TokenArray->iLength > 2) {
            fprintf(stderr, "%s: cd takes one parameter\n", argv[0]);
            return FALSE;
        }
        if (TokenArray->iLength != 1) {
            if (((Token_T)(TokenArray->ppvArray[1]))->type
                                                == TOKEN_EXT) {
            	fprintf(stderr,
                "%s: Pipe or redirection destination not specified\n"
                                                            , argv[0]);
            	return FALSE;
            }
        }
        return TRUE;
    }
    /*exit function don't need argument */
    if (!strcmp(((Token_T)(TokenArray->ppvArray[0]))->str, builtin4)) {
        if (TokenArray->iLength > 1) {
            fprintf(stderr, "%s: exit does not take any parameters\n"
                                                            , argv[0]);
            return FALSE;
        }
        return TRUE;
    }
    /*if command is not built-in command */
    for (i=0; i<TokenArray->iLength; i++) {
        if (((Token_T)(TokenArray->ppvArray[i]))->type == TOKEN_EXT) {
            if (!strcmp(((Token_T)(TokenArray->ppvArray[i]))->str,"|")){
                pipe_exist = TRUE;    //change value to pipe exist
            }
            else {
                redi_exist = TRUE;   //change value to redirection exist
            }
            if (i == 0) {  //first index is pipe
                fprintf(stderr, "%s: Missing command name\n", argv[0]);
                return FALSE;
            }
            if (i == TokenArray->iLength-1) { //last index is pipe
                fprintf(stderr,
                "%s: Pipe or redirection destination not specified\n"
                                                            , argv[0]);
                return FALSE;
            }
            if (((Token_T)(TokenArray->ppvArray[i-1]))->type==TOKEN_EXT
             || ((Token_T)(TokenArray->ppvArray[i+1]))->type==TOKEN_EXT)
            {
                fprintf(stderr,
                "%s: Pipe or redirection destination not specified\n"
                                                            , argv[0]);
                return FALSE;
            }
        }
    }
    if (redi_exist == TRUE) {   //if there are redirection, check these
        enum exist redir_exist = FALSE; //mean no > until now
        enum exist pipe_exist = FALSE;  //mean no | until now
        for (i=0; i<TokenArray->iLength-1; i++) {
            switch(state) {

                case normal:  //send state anything
                    if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, "|")) {
                        state = pipe;
                        backstate = normal;
                    }
                    else if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, "<")) {
                        state = redil;
                        backstate = normal;
                    }
                    else if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, ">")) {
                        state = redir;
                        backstate = normal;
                    }
                    break;
                case pipe:
                    pipe_exist = TRUE;
                    if (redir_exist == TRUE) { //mean > in LHS of |
                        fprintf(stderr,
                        "%s: Multiple redirection of standard in/out\n"
                                                            , argv[0]);
                        return FALSE;
                    }
                    if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, "|")) {
                        state = pipe;
                        backstate = pipe;
                    }
                    else if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, "<")) {
                        state = redil;
                        backstate = pipe;
                    }
                    else if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, ">")) {
                        state = redir;
                        backstate = pipe;
                    }
                    break;
                case redir:
                    redir_exist = TRUE;
                    if (backstate == redir) { //mean there > before >
                        fprintf(stderr,
                        "%s: Multiple redirection of standard in/out\n"
                                                            , argv[0]);
                        return FALSE;
                    }
                    if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, "|")) {
                        state = pipe;
                        backstate = redir;
                    }
                    else if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, "<")) {
                        state = redil;
                        backstate = redir;
                    }
                    else if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, ">")) {
                        state = redir;
                        backstate = redir;
                    }
                    break;
                case redil:
                    /*this mean there is < before < or < in RHS of |*/
                    if (backstate == redil || pipe_exist == TRUE) {
                        fprintf(stderr,
                        "%s: Multiple redirection of standard in/out\n"
                                                            , argv[0]);
                        return FALSE;
                    }
                    if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, "|")) {
                        state = pipe;
                        backstate = redil;
                    }
                    else if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, "<")) {
                        state = redil;
                        backstate = redil;
                    }
                    else if (!strcmp(((Token_T)
                            (TokenArray->ppvArray[i+1]))->str, ">")) {
                        state = redir;
                        backstate = redil;
                    }
                    break;
            }
        }
    }
    return TRUE;
}
