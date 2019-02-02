/*--------------------------------------------------------------------*/
/* mani_execution.c                                                   */
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

enum exist pipe_exist, redi_exist;

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

    /*if command is not built-in and no pipe and no <>*/
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

    /*if command is not built-in and there is pipe and no <>*/
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
    /*if command is not built-in and there is >< */
    else {
        int i, k=1, size, j, iFd;
        int *pipefd;
        pid_t iPid;
        void (*pfRet)(int);

        /*SCArray save info about pipe index in dynamic array*/
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

        /*AuguArray save index of argument that i'th special char and
                i+1'th special char*/
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

            /*if previous special character is pipe */
            if (k != 0 && !strcmp(((Token_T)
                    (TokenArray->ppvArray[SCArray[k]]))->str, "|")) {
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
                if (k == size-1) {
                    free(ArguArray);
                    continue;
                }
                if (iPid == -1) {  //fail
                    perror(ArguArray[0]);
                    free(SCArray);
                    free(pipefd);
                    return FALSE;
                }
            }

            if (k == size-1) continue; //access index k+1 make fault

            if (!strcmp(((Token_T)
                    (TokenArray->ppvArray[SCArray[k+1]]))->str, "|")) {
                /*same as above*/
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
            else if (!strcmp(((Token_T)
                    (TokenArray->ppvArray[SCArray[k+1]]))->str, ">")) {
                fflush(NULL);
                iPid = fork();
                if (iPid == 0) {
                    pfRet = signal(SIGINT, SIG_DFL);
                    assert(pfRet != SIG_ERR);

                    /*open and truncate and creat if there is no file*/
                    iFd = open(((Token_T)
                            (TokenArray->ppvArray[SCArray[k+1]+1]))->str
                                    , O_WRONLY|O_CREAT|O_TRUNC, 0600);
                    dup2(iFd, 1);   /* stdout */
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

                    /*read and if there is no file, return -1*/
                    iFd = open(((Token_T)
                            (TokenArray->ppvArray[SCArray[k+1]+1]))->str
                                                , O_RDONLY, 0600);
                    if (iFd == -1) {
                        fprintf(stderr,"%s: There is no file named %s\n"
                            , argv[0], ((Token_T)(TokenArray->
                                    ppvArray[SCArray[k+1] + 1]))->str);
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
