/**********************
 * EE209 Assignment 1 *
 * 20150073 Kim Gyeong man*
 * file name : client.c *
 **********************/

/* client.c */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_NUM_NAME_CHARS 63
#define MAX_NUM_ID_CHARS 63
#define MAX_NUM_DIGITS   10

/*--------------------------------------------------------------------*/
/* Pre-defined error messages */
#define ERR_UNDEF_CMD   "ERROR: Undefined Command\n"
#define ERR_UNDEF_OPT   "ERROR: Undefined Option\n"
#define ERR_NEED_OPT    "ERROR: Need More Option\n"
#define ERR_SAME_OPT    "ERROR: Multiple Same Options\n"
#define ERR_AMBIG_ARG   "ERROR: Ambiguous Argument\n"
#define ERR_INVALID_ARG "ERROR: Invalid Option Argument\n"

/*--------------------------------------------------------------------*/
enum { FALSE = 0, TRUE };
int c;

typedef enum {
    C_EXIT,       /* exit */
    C_REG,        /* register */
    C_UNREG,      /* unregister */
    C_FIND,       /* find */
    C_FAIL,       /* failure */
    C_EOF         /* end of file */
} Command_T;

/*--------------------------------------------------------------------*/
/* Check whether an exit command valid.                               */
/* Input: no argument                                                 */
/* Return value: TRUE(valid) or FALSE(invalid)                        */
/*--------------------------------------------------------------------*/
static int
ValidateExitCommand(void)
{
    int c;

    /* only blank command is allowed */
    while ((c = getchar()) != EOF && c != '\n' && isspace(c))
        ;
    if (c == EOF)
        exit(0);
    if (c != '\n' && c != EOF) {
        fprintf(stderr, "%s", ERR_UNDEF_OPT);

        /* eat the line */
        while ((c = getchar()) != EOF && c != '\n')
            ;
        if (c == EOF)
            exit(0);
        return FALSE;
    }
    return TRUE;
}

int Validate_case_i(void)
{
    int count_i =0;

    c = getchar();

    if ( !isspace(c) || c == '\n' || c == EOF) {
        fprintf(stderr, "%s", ERR_UNDEF_OPT);
        return FALSE;
    }

    while ((c = getchar()) != EOF && c != '\n' && isspace(c))
        ;

    if (c == '\n' || c == EOF) {
        fprintf(stderr, "%s", ERR_UNDEF_OPT);
        return FALSE;
    }
    while (c != ' ' && c != '\n' && c != EOF ) {
        if ( isalpha(c) || isdigit(c) || c == '-' || c == '_' || c == '.') {
            count_i++;
            if (count_i > MAX_NUM_ID_CHARS) {
                fprintf(stderr, "%s", ERR_INVALID_ARG);
                return FALSE;
            }
        }

        else {
            fprintf(stderr, "%s", ERR_INVALID_ARG);
            return FALSE;
        }

        c = getchar();
    }

    if (c != '\n' && c != EOF) {
        while ((c = getchar()) != EOF && c != '\n' && isspace(c))
            ;
    }
    return TRUE;
}

int Validate_case_n(void)
{
    int count_n = 0;
    enum DFAState {normal, backslash, quote};
    enum DFAState state = normal;
    enum DFAState backstate = normal;

    c = getchar();

    if ( !isspace(c) || c == '\n' || c == EOF) {
        fprintf(stderr, "%s", ERR_UNDEF_OPT);
        return FALSE;
    }

    while ((c = getchar()) != EOF && c != '\n' && isspace(c))
        ;

    if (c == '\n' || c == EOF) {
        fprintf(stderr, "%s", ERR_UNDEF_OPT);
        return FALSE;
    }

    if (c == '\'') {
        state = quote;
        c = getchar();
    }

    while (c != '\n' && c != EOF && (c != ' ' || state != normal)) {
        switch (state) {

            case normal:
                if (backstate == quote) {
                    fprintf(stderr, "%s", ERR_INVALID_ARG);
                    return FALSE;
                }

                else if (c == '\\') {
                    state = backslash;
                    backstate = normal;
                }
                else if ( isalpha(c) || c == '-' || c == '.' || c == ' ' || c == '\'') {
                    count_n++;
                }
                else {
                    fprintf(stderr, "%s", ERR_INVALID_ARG);
                    return FALSE;
                }
                break;

            case backslash:
                if ( isalpha(c) || c == '-' || c == '.' || c == ' ' || c == '\'') {
                    count_n++;
                }
                else {
                    fprintf(stderr, "%s", ERR_INVALID_ARG);
                    return FALSE;
                }

                state = backstate;
                break;

            case quote:
                if (c == '\'') {
                    state = normal;
                    backstate = quote;
                }
                else if (c == '\\') {
                    state = backslash;
                    backstate = quote;
                }
                else if ( isalpha(c) || c == '-' || c == '.' || c == ' ') {
                    count_n++;
                }
                else {
                    fprintf(stderr, "%s", ERR_INVALID_ARG);
                    return FALSE;
                }

                break;
        }

        c = getchar();
    }

    if (state == quote) {
         fprintf(stderr, "%s", ERR_INVALID_ARG);
        return FALSE;
    }

    if (count_n > MAX_NUM_NAME_CHARS || count_n <1) {
        fprintf(stderr, "%s", ERR_INVALID_ARG);
        return FALSE;
    }

    if (c != '\n' && c != EOF) {
        while ((c = getchar()) != EOF && c != '\n' && isspace(c))
            ;
    }
    return TRUE;
}

int Validate_case_p(void)
{
    int count_p = 0;
    typedef enum {FALSE, TRUE} bool;
    bool count_firstzero = FALSE;
    char* point = NULL;
    const char* digits = "0123456789";

    c = getchar();

    if ( !isspace(c) || c == '\n' || c == EOF) {
        fprintf(stderr, "%s", ERR_UNDEF_OPT);
        return FALSE;
    }

    while ((c = getchar()) != EOF && c != '\n' && isspace(c))
        ;

    if (c == '\n' || c == EOF) {
        fprintf(stderr, "%s", ERR_UNDEF_OPT);
        return FALSE;
    }

    if (c == '0') {
        count_firstzero = TRUE;
    }

    while (c != ' ' && c != '\n' && c != EOF) {
        if ( (point = strchr(digits, c)) != NULL) {
            count_p++;
            if (count_firstzero && count_p > 1) {
                fprintf(stderr, "%s", ERR_INVALID_ARG);
                return FALSE;
            }
            if (count_p > MAX_NUM_DIGITS) {
                fprintf(stderr, "%s", ERR_INVALID_ARG);
                return FALSE;
            }
        }

        else {
            fprintf(stderr, "%s", ERR_INVALID_ARG);
            return FALSE;
        }

        c = getchar();
    }

    if (c != '\n' && c != EOF) {
        while ((c = getchar()) != EOF && c != '\n' && isspace(c))
            ;
    }
    return TRUE;
}


static int
ValidateRegisterCommand(void)
{
    int check_i = 0, check_n=0, check_p=0;

    while ((c = getchar()) != EOF && c != '\n' && isspace(c))
        ;

    while (c != '\n' && c != EOF) {
        if (c == '-') {
            c = getchar();
            switch (c) {
                case 'i':
                    if (Validate_case_i() == FALSE) return FALSE;

                    check_i++;
                    break;

                case 'n':
                    if (Validate_case_n() == FALSE) return FALSE;

                    check_n++;
                    break;

                case 'p':
                    if (Validate_case_p() == FALSE) return FALSE;

                    check_p++;
                    break;

                default:
                    fprintf(stderr, "%s", ERR_UNDEF_OPT);
                    return FALSE;
            }
            if (check_i > 1 ||  check_n > 1 || check_p > 1) {
                fprintf(stderr, "%s", ERR_SAME_OPT);
                return FALSE;
            }
        }
        else {
            fprintf(stderr, "%s", ERR_UNDEF_OPT);
            return FALSE;
        }

    }

    if (check_i *check_n * check_p == 0) {
        fprintf(stderr, "%s", ERR_NEED_OPT);
        return FALSE;
    }

    return TRUE;
}

static int
ValidateUnregisterOrFindCommand(void)
{
    int check_i = 0, check_n=0;
    while ((c = getchar()) != EOF && c != '\n' && isspace(c))
        ;

    while (c != '\n' && c != EOF) {
        if (c == '-') {
            c = getchar();
            switch (c) {
                case 'i':
                    if (Validate_case_i() == FALSE) return FALSE;

                    check_i++;
                    break;

                case 'n':
                    if (Validate_case_n() == FALSE) return FALSE;

                    check_n++;
                    break;

                default:
                    fprintf(stderr, "%s", ERR_UNDEF_OPT);
                    return FALSE;
            }
            if (check_i + check_n != 1) {
                if (check_i == 1 && check_n == 1) {
                    fprintf(stderr, "%s", ERR_AMBIG_ARG);
                    return FALSE;
                }
                else if (check_i > 1 ||  check_n > 1) {
                    fprintf(stderr, "%s", ERR_SAME_OPT);
                    return FALSE;
                }
            }
        }
        else {
            fprintf(stderr, "%s", ERR_UNDEF_OPT);
            return FALSE;
        }

    }

    if (check_i *check_n == 0) {
        fprintf(stderr, "%s", ERR_NEED_OPT);
        return FALSE;
    }

    return TRUE;
}

static Command_T
GetCommandType(void)
{
    Command_T type = C_FAIL;
    const char *cmds[] = {
        "exit",   /* exit */
        "reg",    /* reg */
        "unreg",  /* unreg */
        "find",   /* find */
    };
    int i, len;

    /* eat space */
    while ((c = getchar()) != EOF && c != '\n' && isspace(c))
        ;

    switch (c) {
        case '\n':return C_FAIL;  /* no command */
        case EOF: return C_EOF;   /* no more input */
        case 'e': type = C_EXIT;  break;
        case 'r': type = C_REG;   break;
        case 'u': type = C_UNREG; break;
        case 'f': type = C_FIND;  break;
        default:
            fprintf(stderr, "%s", ERR_UNDEF_CMD);
            goto EatLineAndReturn;
    }

    /* see the rest of the command chars actually match */
    len = strlen(cmds[type]);
    for (i = 1; i < len; i++) {
        c = getchar();
        if (c == '\n' || c == EOF) {   /* line finished too early */
            fprintf(stderr, "%s", ERR_UNDEF_CMD);
            return (c == EOF) ? C_EOF : C_FAIL;
        }
        if (c != cmds[type][i]) {    /* wrong command */
            fprintf(stderr, "%s", ERR_UNDEF_CMD);
            goto EatLineAndReturn;
        }
    }

    /* check the following character of a command */
    c = getchar();
    if (c != '\n' && isspace(c)) {
        return type;
    } else if (c == '\n' || c == EOF) {
        /* only exit can be followed by '\n' */
        if (type != C_EXIT)
            fprintf(stderr, "%s", ERR_NEED_OPT);
        if (c == EOF)
            return C_EOF;
        else
            ungetc(c, stdin);
        if (type == C_EXIT)
            return type;
    } else {
        fprintf(stderr, "%s", ERR_UNDEF_CMD);
    }

EatLineAndReturn:
    while ((c = getchar()) != EOF && (c != '\n'))
        ;
    return (c == EOF) ? C_EOF : C_FAIL;
}
/*--------------------------------------------------------------------*/
int
main(int argc, const char *argv[])
{
    Command_T command;
    int res;
    printf("======================================\n" \
           "  Customer Manager Program\n" \
           "======================================\n\n");

    /* start prompt */
    while (TRUE) {
        printf("\n> ");

        /* check command type */
        command = GetCommandType();

        /* command validation */
        switch (command) {
            case C_EOF:
                return 0;
            case C_FAIL:
                res = FALSE;
                break;
            case C_EXIT:
                res = ValidateExitCommand();
                break;
            case C_REG:
                res = ValidateRegisterCommand();
                break;
            case C_FIND:
                res = ValidateUnregisterOrFindCommand();
                break;
            case C_UNREG:
                res = ValidateUnregisterOrFindCommand();
                break;
            default:
                assert(0); /* can't reach here */
                break;
        }

        if (res == FALSE) {
            /* validation fail */
            if ( command != C_EXIT && command != C_FAIL && c != '\n') {
                while ((c = getchar()) != EOF && (c != '\n'))
                    ;
            }
            continue;
        }

        /* command functionalities */
        switch (command) {
            case C_EXIT:
                exit(0);
                return 0;
            default:
                /* This will be expanded in assignment 3. */
                break;
        }
    }
    assert(0);  /* can't reach here */
    return 0;
}
