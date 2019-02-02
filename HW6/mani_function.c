/*--------------------------------------------------------------------*/
/* mani_function.c                                                    */
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
#include <signal.h>
#include<sys/socket.h>
#include<fcntl.h>

#include "mani_ish.h"

enum exist pipe_exist, redi_exist; //when there is pipe in line, change into TRUE

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
    if (isspace(Line[0]) || Line[0] == '|' || Line[0] == '<' || Line[0] == '>') state = malloc;
    if (Line[0] == '\"' || Line[0] == '\'') state = quote;
    length = strlen(Line);

    for (i=0; i<length; i++) {
        switch(state) {

            case normal:
                if (backstate == malloc) {   //word start
                    start = &Line[i];
                    count = 0;
                }
                if (Line[i+1] == '\"' || Line[i+1] == '\'') state = quote;
                else if (isspace(Line[i+1]) || Line[i+1] == '|' || Line[i+1] == '<' || Line[i+1] == '>') state = malloc;

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
                    else if (isspace(Line[i+1]) || Line[i+1] == '|' || Line[i+1] == '\0'  || Line[i+1] == '<' || Line[i+1] == '>') state = malloc;
                    continue;
                }
                if (backstate == malloc && (Line[i] == '|'  || Line[i] == '<' || Line[i] == '>')) {  //pipe
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
                    else if (isspace(Line[i+1]) || Line[i+1] == '|' || Line[i+1] == '\0' || Line[i+1] == '<' || Line[i+1] == '>')state= malloc;
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

                if (Line[i] == '|' || Line[i] == '<' || Line[i] == '>') {    //pipe
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
                else if (isspace(Line[i+1]) || Line[i+1] == '|' || Line[i+1] == '\0' || Line[i+1] == '<' || Line[i+1] == '>') state = malloc;

                backstate = malloc;
                break;
        }
    }
    /*when finish for() */
    if (state == quote) {
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
    redi_exist = FALSE;
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
            if (!strcmp(((Token_T)(TokenArray->ppvArray[i]))->str, "|")) {
                pipe_exist = TRUE;    //change value to represent pipe exist
            }
            else {
                redi_exist = TRUE;   //change value to represent redirection exist
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
    if (redi_exist == TRUE) {
        enum exist redir_exist = FALSE;
        enum exist pipe_exist = FALSE;
        for (i=0; i<TokenArray->iLength-1; i++) {
            switch(state) {

                case normal:
                    if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, "|")) {
                        state = pipe;
                        backstate = normal;
                    }
                    else if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, "<")) {
                        state = redil;
                        backstate = normal;
                    }
                    else if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, ">")) {
                        state = redir;
                        backstate = normal;
                    }
                    break;
                case pipe:
                    pipe_exist = TRUE;
                    if (redir_exist == TRUE) {
                        fprintf(stderr, "%s: Multiple redirection of standard in/out\n", argv[0]);
                        return FALSE;
                    }
                    if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, "|")) {
                        state = pipe;
                        backstate = pipe;
                    }
                    else if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, "<")) {
                        state = redil;
                        backstate = pipe;
                    }
                    else if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, ">")) {
                        state = redir;
                        backstate = pipe;
                    }
                    break;
                case redir:
                    redir_exist = TRUE;
                    if (backstate == redir) {
                        fprintf(stderr, "%s: Multiple redirection of standard in/out\n", argv[0]);
                        return FALSE;
                    }
                    if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, "|")) {
                        state = pipe;
                        backstate = redir;
                    }
                    else if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, "<")) {
                        state = redil;
                        backstate = redir;
                    }
                    else if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, ">")) {
                        state = redir;
                        backstate = redir;
                    }
                    break;
                case redil:
                    if (backstate == redil || pipe_exist == TRUE) {
                        fprintf(stderr, "%s: Multiple redirection of standard in/out\n", argv[0]);
                        return FALSE;
                    }
                    if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, "|")) {
                        state = pipe;
                        backstate = redil;
                    }
                    else if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, "<")) {
                        state = redil;
                        backstate = redil;
                    }
                    else if (!strcmp(((Token_T)(TokenArray->ppvArray[i+1]))->str, ">")) {
                        state = redir;
                        backstate = redil;
                    }
                    break;
            }
        }
    }
    return TRUE;
}
/*--------------------------------------------------------------------*/
/* execution function execute one line. First, if there are built-in
    function, then execute appropriate function with argument in
    dynamic array. Second, If there is no built-in function but no
    pipe, then execute with execvp function and folk function for using
    child process. Finally, there are pipe and no built-in function,
    use for() and use folk function and pipe function for each for()
    iteration. So i iteration's output is into pipe[i-1][1] and
    i+1 iteration's input is into that pipe[i-1][0]. Return TRUE when
    success execute and return FALSE when there are any error*/
int execution( DynArray_T TokenArray, char *argv[]) {
    assert(TokenArray != NULL);
    assert(TokenArray->ppvArray != NULL);
    assert(TokenArray->iLength >= 0);
    char *builtin1 = "setenv";
    char *builtin2 = "unsetenv";
    char *builtin3 = "cd";
    char *builtin4 = "exit";
    int result;

    /*setenv*/
    if (!strcmp(((Token_T)(TokenArray->ppvArray[0]))->str, builtin1)) {
        if (TokenArray->iLength == 2) {
            result = setenv(((Token_T)(TokenArray->ppvArray[1]))->str
                                                            , "", 1);
        }
        else if (TokenArray->iLength == 3) {
            result = setenv(((Token_T)(TokenArray->ppvArray[1]))->str
                        , ((Token_T)(TokenArray->ppvArray[2]))->str,1);
        }
        if (result == 0) return TRUE;
        else if (result == -1) return FALSE;
    }
    /*unsetenv*/
    if (!strcmp(((Token_T)(TokenArray->ppvArray[0]))->str, builtin2)) {
        result = unsetenv(((Token_T)(TokenArray->ppvArray[1]))->str);
        if (result == 0) return TRUE;
        else if (result == -1) return FALSE;
    }
    /*cd, using getenv function for home directory*/
    if (!strcmp(((Token_T)(TokenArray->ppvArray[0]))->str, builtin3)) {
        if (TokenArray->iLength == 1) {
            result = chdir(getenv("HOME"));
            return TRUE;
        }
        result = chdir(((Token_T)(TokenArray->ppvArray[1]))->str);
        if (result == 0) return TRUE;
        else if (result == -1) {
            fprintf(stderr, "%s: No such file or directory\n", argv[0]);
            return FALSE;
        }
    }
    /*exit*/
    if (!strcmp(((Token_T)(TokenArray->ppvArray[0]))->str, builtin4)) {
        TokenArray_free(TokenArray);
        exit(0);
    }

    /*if command is not built-in and no pipe in line*/
    if (pipe_exist == FALSE && redi_exist == FALSE) {
        pid_t iPid;
        int i;
        void (*pfRet)(int);

        char** ArguArray = (char**)calloc((size_t)TokenArray->iLength+1
                                                        ,sizeof(char*));
        if (ArguArray == NULL) {
            fprintf(stderr, "%s: fail to allocate\n", argv[0]);
            return FALSE;
        }
        for(i=0; i<TokenArray->iLength; i++) { //make argument array
            ArguArray[i] = ((Token_T)(TokenArray->ppvArray[i]))->str;
        }
        fflush(NULL);
        iPid = fork();   //make child process and execute command

        if (iPid == 0) {
            pfRet = signal(SIGINT, SIG_DFL); //no ignore SIGINT
            assert(pfRet != SIG_ERR);

            execvp(ArguArray[0], ArguArray); //execute
            perror(((Token_T)(TokenArray->ppvArray[0]))->str);
            exit(0);
        }
        iPid = wait(NULL);
        free(ArguArray);
        if (iPid == -1) {
            perror(((Token_T)(TokenArray->ppvArray[0]))->str);
            return FALSE;
        }
        return TRUE;
    }

    /*if command is not built-in and there is pipe in line*/
    else if (pipe_exist == TRUE && redi_exist == FALSE) {
        int i, k=1, size, j;
        int **pipefd;
        pid_t iPid;
        void (*pfRet)(int);

        /*PipeArray save info about pipe index in dynamic array*/
        int* PipeArray = (int*)calloc(TokenArray->iLength,sizeof(int));
        if (PipeArray == NULL) {
            fprintf(stderr, "%s: fail to allocate\n", argv[0]);
            return FALSE;
        }
        char** ArguArray;

        PipeArray[0] = -1;
        for (i=0; i<TokenArray->iLength; i++) {
            if (((Token_T)(TokenArray->ppvArray[i]))->type==TOKEN_EXT){
                PipeArray[k] = i;
                k++;
            }
        }
        PipeArray[k] = TokenArray->iLength;
        size = k;

        /*pipefd is 2 dimension array that will use in pipe function*/
        pipefd = (int**) malloc( sizeof(int*) * (size-1) );
        if (pipefd == NULL) {
            fprintf(stderr, "%s: fail to allocate\n", argv[0]);
            free(PipeArray);
            return FALSE;
        }
        for(i=0; i<(size-1); i++){
            pipefd[i] = (int*) malloc ( sizeof(int) * 2);
            if (pipefd[i] == NULL) {
                fprintf(stderr, "%s: fail to allocate\n", argv[0]);
                free(PipeArray);
                for (j=0; j<i; j++) {
                    free(pipefd[j]);
                }
                free(pipefd);
                return FALSE;
            }
        }

        /*AuguArray save index of argument that i'th pipe and
                i+1'th pipe*/
        for (k=0; k<size; k++) {
            ArguArray = (char**)calloc((size_t)TokenArray->iLength+1
                                                    , sizeof(char*));
            if (ArguArray == NULL) {
                fprintf(stderr, "%s: fail to allocate\n", argv[0]);
                free(PipeArray);
                for (i=0; i<(size-1); i++){
                    free(pipefd[i]);
                }
                free(pipefd);
                return FALSE;
            }

            j=0;
            for (i=PipeArray[k] + 1; i<PipeArray[k+1]; i++) {
                ArguArray[j]=((Token_T)(TokenArray->ppvArray[i]))->str;
                j++;
            }

            /*when execute command before first pipe*/
            if (k == 0) {
                if (pipe(pipefd[k]) == -1) {
                    fprintf(stderr, "%s: fail to allocate pipefd\n"
                                                        , argv[0]);
                    free(PipeArray);
                    for (i=0; i<(size-1); i++){
                        free(pipefd[i]);
                    }
                    free(pipefd);
                    free(ArguArray);
                    return FALSE;
                }

                fflush(NULL);
                iPid = fork();
                if (iPid == 0) {   //child process
                    pfRet = signal(SIGINT, SIG_DFL);
                    assert(pfRet != SIG_ERR);
                    close(pipefd[k][0]);
                    dup2(pipefd[k][1], 1);  /* stdout */
                    close(pipefd[k][1]);
                    execvp(ArguArray[0], ArguArray);
                    perror(ArguArray[0]);
                    exit(0);
                }
                iPid = wait(NULL);
                free(ArguArray);
                if (iPid == -1) { //fail
                    perror(ArguArray[0]);
                    free(PipeArray);
                    for (i=0; i<(size-1); i++){
                        free(pipefd[i]);
                    }
                    free(pipefd);
                    return FALSE;
                }
            }

            /*when execute command after last pipe*/
            else if (k == size-1) {
                fflush(NULL);
                iPid = fork();
                if (iPid == 0) {   //child process
                    pfRet = signal(SIGINT, SIG_DFL);
                    assert(pfRet != SIG_ERR);
                    close(pipefd[k-1][1]);
                    dup2(pipefd[k-1][0], 0); /* stdin */
                    close(pipefd[k-1][0]);
                    execvp(ArguArray[0], ArguArray);
                    perror(ArguArray[0]);
                    exit(0);
                }
                for (i=0; i<k; i++) { //close pre pipe
                    close(pipefd[i][0]);
                    close(pipefd[i][1]);
                }
                iPid = wait(NULL);
                free(ArguArray);
                if (iPid == -1) {  //fail
                    perror(ArguArray[0]);
                    free(PipeArray);
                    for (i=0; i<(size-1); i++){
                        free(pipefd[i]);
                    }
                    free(pipefd);
                    return FALSE;
                }
            }

            /*when execute command between pipes*/
            else {
                if (pipe(pipefd[k]) == -1) {
                    fprintf(stderr, "%s: fail to allocate pipefd\n"
                                                            , argv[0]);
                    free(PipeArray);
                    for (i=0; i<(size-1); i++){
                        free(pipefd[i]);
                    }
                    free(pipefd);
                    free(ArguArray);
                    return FALSE;
                }

                fflush(NULL);
                iPid = fork();
                if (iPid == 0) {   //child process
                    pfRet = signal(SIGINT, SIG_DFL);
                    assert(pfRet != SIG_ERR);
                    close(pipefd[k-1][1]);
                    dup2(pipefd[k-1][0], 0); /* stdin */
                    close(pipefd[k-1][0]);
                    close(pipefd[k][0]);
                    dup2(pipefd[k][1], 1); /* stdout */
                    close(pipefd[k][1]);

                    execvp(ArguArray[0], ArguArray);
                    perror(ArguArray[0]);
                    exit(0);
                }
                for (i=0; i<k; i++) { //close pre pipe
                    close(pipefd[i][0]);
                    close(pipefd[i][1]);
                }
                iPid = wait(NULL);
                free(ArguArray);
                if (iPid == -1) {  //fail
                    perror(ArguArray[0]);
                    fprintf(stderr, "%s: fail to allocate pipefd\n"
                                                        , argv[0]);
                    free(PipeArray);
                    for (i=0; i<(size-1); i++){
                        free(pipefd[i]);
                    }
                    free(pipefd);
                    return FALSE;
                }
            }
        }
        /*when finish execute, we should free*/
        for(i=0; i<(size-1); i++){
            free(pipefd[i]);
        }
        free(pipefd);
        free(PipeArray);
        return TRUE;
    }
    else {
        int i, k=1, size, j, iFd;
        int *pipefd;
        pid_t iPid;
        void (*pfRet)(int);

        /*PipeArray save info about pipe index in dynamic array*/
        int* SCArray = (int*)calloc(TokenArray->iLength,sizeof(int));
        if (SCArray == NULL) {
            fprintf(stderr, "%s: fail to allocate\n", argv[0]);
            return FALSE;
        }
        char** ArguArray;

        SCArray[0] = -1;
        for (i=0; i<TokenArray->iLength; i++) {
            if (((Token_T)(TokenArray->ppvArray[i]))->type==TOKEN_EXT){
                SCArray[k] = i;
                k++;
            }
        }
        SCArray[k] = TokenArray->iLength;
        size = k;

        /*pipefd is 2 dimension array that will use in pipe function*/
        pipefd = (int*) malloc(sizeof(int) * 2);
        if (pipefd == NULL) {
            fprintf(stderr, "%s: fail to allocate\n", argv[0]);
            free(SCArray);
            return FALSE;
        }

        /*AuguArray save index of argument that i'th pipe and
                i+1'th pipe*/
        for (k=0; k<size; k++) {
            ArguArray = (char**)calloc((size_t)TokenArray->iLength+1
                                                    , sizeof(char*));
            if (ArguArray == NULL) {
                fprintf(stderr, "%s: fail to allocate\n", argv[0]);
                free(SCArray);
                free(pipefd);
                return FALSE;
            }

            j=0;
            for (i=SCArray[k] + 1; i<SCArray[k+1]; i++) {
                ArguArray[j]=((Token_T)(TokenArray->ppvArray[i]))->str;
                j++;
            }

            if (k != 0 && !strcmp(((Token_T)(TokenArray->ppvArray[SCArray[k]]))->str, "|")) {
                fflush(NULL);
                iPid = fork();
                if (iPid == 0) {   //child process
                    pfRet = signal(SIGINT, SIG_DFL);
                    assert(pfRet != SIG_ERR);
                    close(pipefd[1]);
                    dup2(pipefd[0], 0); /* stdin */
                    close(pipefd[0]);
                    execvp(ArguArray[0], ArguArray);
                    perror(ArguArray[0]);
                    exit(0);
                }
                close(pipefd[0]);
                close(pipefd[1]);
                iPid = wait(NULL);
                free(ArguArray);
                if (iPid == -1) {  //fail
                    perror(ArguArray[0]);
                    free(SCArray);
                    free(pipefd);
                    return FALSE;
                }
            }

            if (k == size-1) continue;

            if (!strcmp(((Token_T)(TokenArray->ppvArray[SCArray[k+1]]))->str, "|")) {
                if (pipe(pipefd) == -1) {
                    fprintf(stderr, "%s: fail to allocate pipefd\n"
                                                        , argv[0]);
                    free(SCArray);
                    free(pipefd);
                    free(ArguArray);
                    return FALSE;
                }
                fflush(NULL);
                iPid = fork();
                if (iPid == 0) {   //child process
                    pfRet = signal(SIGINT, SIG_DFL);
                    assert(pfRet != SIG_ERR);
                    close(pipefd[0]);
                    dup2(pipefd[1], 1);  /* stdout */
                    close(pipefd[1]);
                    execvp(ArguArray[0], ArguArray);
                    perror(ArguArray[0]);
                    exit(0);
                }
                iPid = wait(NULL);
                free(ArguArray);
                if (iPid == -1) { //fail
                    perror(ArguArray[0]);
                    free(SCArray);
                    free(pipefd);
                    return FALSE;
                }
            }
            else if (!strcmp(((Token_T)(TokenArray->ppvArray[SCArray[k+1]]))->str, ">")) {
                fflush(NULL);
                iPid = fork();
                if (iPid == 0) {
                    pfRet = signal(SIGINT, SIG_DFL);
                    assert(pfRet != SIG_ERR);

                    iFd = open(((Token_T)(TokenArray->ppvArray[SCArray[k+1] + 1]))->str, O_WRONLY, 0600);
                    dup2(iFd, 1);  /* stdout */
                    close(iFd);

                    execvp(ArguArray[0], ArguArray);
                    perror(ArguArray[0]);
                    exit(0);
                }
                iPid = wait(NULL);
                free(ArguArray);
                if (iPid == -1) { //fail
                    perror(ArguArray[0]);
                    free(SCArray);
                    free(pipefd);
                    return FALSE;
                }
            }
            else {
                fflush(NULL);
                iPid = fork();
                if (iPid == 0) {
                    pfRet = signal(SIGINT, SIG_DFL);
                    assert(pfRet != SIG_ERR);

                    iFd = open(((Token_T)(TokenArray->ppvArray[SCArray[k+1] + 1]))->str, O_RDONLY, 0600);
                    if (iFd == -1) {
                        fprintf(stderr, "%s: There is no file named %s\n", argv[0], ((Token_T)(TokenArray->ppvArray[SCArray[k+1] + 1]))->str);
                    }
                    dup2(iFd, 0);  /* stdout */
                    close(iFd);

                    execvp(ArguArray[0], ArguArray);
                    perror(ArguArray[0]);
                    exit(0);
                }
                iPid = wait(NULL);
                free(ArguArray);
                if (iPid == -1) {  //fail
                    perror(ArguArray[0]);
                    free(SCArray);
                    free(pipefd);
                    return FALSE;
                }
            }
        }
        /*when finish execute, we should free*/
        free(pipefd);
        free(SCArray);
        return TRUE;
    }
    return TRUE;
}
/*--------------------------------------------------------------------*/
/* TokenArray_free function is called when finished while() in main.
    we should free dynamic array and Token structure elementin that
    array. */
void TokenArray_free(DynArray_T TokenArray) {
    assert(TokenArray != NULL);
    assert(TokenArray->ppvArray != NULL);
    assert(TokenArray->iLength >= 0);
    int i;
    for (i=0; i<TokenArray->iLength; i++) {
        free(((Token_T)(TokenArray->ppvArray[i]))->str);
        free(TokenArray->ppvArray[i]);
    }
    DynArray_free(TokenArray);
}
/*--------------------------------------------------------------------*/
/* mySigquitHandler and mySigalrmHandler function exist for SIGQUIT
    signal. When SIGQUIT signal received, execute alarm(5) and increase
    count_sig. And print sentence. When receive SIGQUIT before 5
    seconds, sig_count - value go up to 2 so we should exit. */
int count_sig =0;
int value = 0;
void mySigquitHandler (int iSignal) {
    alarm(5);
    count_sig++;
    if (count_sig - value == 2) {
        exit(0);
    }
    fprintf(stdout, "Type Ctrl-\\ again within 5 seconds to exit.\n");
}

void mySigalrmHandler (int iSignal) {
     value++;
}
