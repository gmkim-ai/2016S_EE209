/*--------------------------------------------------------------------*/
/* mani_ish.h                                                         */
/* Author: Gyeongman Kim  20150073                                    */
/*--------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>
#include "dynarray.h"

#ifndef _MANI_ISH_H_
#define _MANI_ISH_H_

#pragma once

extern enum exist { FALSE = 0, TRUE } pipe_exist, redi_exist;
/*--------------------------------------------------------------------*/
/* divide ordinary and extraordinary word, so | is TOKEN_EXT and
    "|" is TOKEN_ORD */
enum TokenType { TOKEN_ORD = 0, TOKEN_EXT };

struct Token {
     /* The type of the token. */
     enum TokenType type;

     /* The string which is the token's value. */
     char *str;
};

/*--------------------------------------------------------------------*/
/* These functions's comment is in shell.c and mani_ish.c */
typedef struct Token * Token_T;

void TokenArray_free(DynArray_T TokenArray);

int execution( DynArray_T TokenArray, char *argv[]);

int syntactic( DynArray_T TokenArray, char *argv[]);

int lexical( DynArray_T TokenArray, char *Line, char *argv[]);

void mySigquitHandler (int iSignal);

void mySigalrmHandler (int iSignal);

#endif // _MANI_ISH_H_
