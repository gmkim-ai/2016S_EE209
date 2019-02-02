/*--------------------------------------------------------------------*/
/* mani_ish.c                                                         */
/* Author: Gyeongman Kim  20150073                                    */
/*--------------------------------------------------------------------*/
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "mani_ish.h"
/*--------------------------------------------------------------------*/
/* First, handling about signal so ignore SIGINT and make handler
    function about  SIGQUIT and SIGALRM. Then make function pointer
    that path is home directory. So read one line and print, and put
    in three step function named lexical, syntactic, execution.
    Finally, wait for input in stdin and process it directly with
    three step function. */
int main(int argc, char *argv[]) {
    void (*pfRet)(int);
    sigset_t sSet;
    sigemptyset(&sSet);
    sigaddset(&sSet, SIGINT);
    sigaddset(&sSet, SIGALRM);
    sigaddset(&sSet, SIGQUIT);
    sigprocmask(SIG_UNBLOCK, &sSet, NULL);
    pfRet = signal(SIGINT, SIG_IGN);
    assert(pfRet != SIG_ERR);

    pfRet = signal(SIGQUIT, mySigquitHandler);
    assert(pfRet != SIG_ERR);
    pfRet = signal(SIGALRM, mySigalrmHandler);
    assert(pfRet != SIG_ERR);

    int res;
    DynArray_T oDynArray;
    char arr[1024];
    FILE *fp;
    char *home = strdup(getenv("HOME"));

    fp = fopen(strcat(home, "/.ishrc"), "r");
    free(home);
    if (fp == NULL)
    {
        fprintf(stderr, "There is no .ishrc in home directory\n");
    }
    else {  //read file and process data with three steps
        while (fgets(arr, sizeof(arr), fp)) {
            fprintf(stdout, "%% %s", arr);
            oDynArray = DynArray_new(0);
            res = lexical(oDynArray, arr, argv);
            if (res == TRUE) {    //go to next step when success
                res = syntactic(oDynArray, argv);
            }
            if (res == TRUE) {    //go to next step when success
                res = execution(oDynArray, argv);
            }
            TokenArray_free(oDynArray);
        }
        fclose(fp);
    }

    while(1) {   //wait for input in stdin
        oDynArray = DynArray_new(0);
        fgets(arr, sizeof(arr), stdin);
        res = lexical(oDynArray, arr, argv);
        if (res == TRUE) {
            res = syntactic(oDynArray, argv);
        }
        if (res == TRUE) {
            res = execution(oDynArray, argv);
        }
        TokenArray_free(oDynArray);
    }
    return 0;
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
