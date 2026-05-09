#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "dfa.h"
#include "tokenizer.h"

/* -----------------------------------------------------------------------
 * RegisterDFA
 * States:
 *   1 = start           (expecting 'R')
 *   2 = saw 'R'         (expecting digit 0-9)
 *   3 = accepting       (saw R + one digit)
 *   4 = trap
 * ----------------------------------------------------------------------- */
int register_validate(const char *input) {
    int state = 1;

    for (int i = 0; input[i] != '\0'; i++) {
        char c = (char)toupper((unsigned char)input[i]);

        switch (state) {
            case 1:
                if (c == 'R') state = 2;
                else          state = 4;
                break;

            case 2:
                if (c >= '0' && c <= '9') state = 3;
                else                      state = 4;
                break;

            case 3:
                /* Any extra character after R+digit → trap */
                state = 4;
                break;

            case 4:
                return 0;   /* early-exit: already in trap */
        }
    }

    return (state == 3);
}

/* -----------------------------------------------------------------------
 * NumberDFA
 * States:
 *   1 = start           (expecting '-' or digit)
 *   2 = saw '-'         (must see digit)
 *   3 = accepting       (saw at least one digit)
 *   4 = trap
 * ----------------------------------------------------------------------- */
int number_validate(const char *input) {
    if (input == NULL || input[0] == '\0') return 0;

    int state = 1;

    for (int i = 0; input[i] != '\0'; i++) {
        char c = input[i];

        switch (state) {
            case 1:
                if (c == '-')                 state = 2;
                else if (c >= '0' && c <= '9') state = 3;
                else                           state = 4;
                break;

            case 2:
                state = (c >= '0' && c <= '9') ? 3 : 4;
                break;

            case 3:
                state = (c >= '0' && c <= '9') ? 3 : 4;
                break;

            case 4:
                state = 4;
                break;
        }
    }

    return (state == 3);
}

/* -----------------------------------------------------------------------
 * OpcodeDFA  – faithful port of the Java state machine
 *
 *  State map (same numbering as Java):
 *   1  = start
 *   2  = saw M
 *   3  = saw A
 *   4  = saw S
 *   5  = saw I
 *   6  = saw D
 *   7  = saw N
 *   8  = saw H
 *   9  = saw MO
 *   10 = saw AD
 *   11 = saw SU
 *   12 = saw IN
 *   13 = saw DE
 *   14 = saw NO
 *   15 = saw HL
 *   16 = accepting: MOV
 *   17 = accepting: ADD
 *   18 = accepting: SUB
 *   19 = accepting: INC
 *   20 = accepting: DEC
 *   21 = accepting: NOP
 *   22 = accepting: HLT
 *   23 = trap
 * ----------------------------------------------------------------------- */
int opcode_validate(const char *input) {
    int state = 1;

    for (int i = 0; input[i] != '\0'; i++) {
        char c = (char)toupper((unsigned char)input[i]);

        switch (state) {
            case 1:
                if      (c == 'M') state = 2;
                else if (c == 'A') state = 3;
                else if (c == 'S') state = 4;
                else if (c == 'I') state = 5;
                else if (c == 'D') state = 6;
                else if (c == 'N') state = 7;
                else if (c == 'H') state = 8;
                else               state = 23;
                break;

            /* MOV */
            case 2:  state = (c == 'O') ? 9  : 23; break;
            case 9:  state = (c == 'V') ? 16 : 23; break;

            /* ADD */
            case 3:  state = (c == 'D') ? 10 : 23; break;
            case 10: state = (c == 'D') ? 17 : 23; break;

            /* SUB */
            case 4:  state = (c == 'U') ? 11 : 23; break;
            case 11: state = (c == 'B') ? 18 : 23; break;

            /* INC */
            case 5:  state = (c == 'N') ? 12 : 23; break;
            case 12: state = (c == 'C') ? 19 : 23; break;

            /* DEC */
            case 6:  state = (c == 'E') ? 13 : 23; break;
            case 13: state = (c == 'C') ? 20 : 23; break;

            /* NOP */
            case 7:  state = (c == 'O') ? 14 : 23; break;
            case 14: state = (c == 'P') ? 21 : 23; break;

            /* HLT */
            case 8:  state = (c == 'L') ? 15 : 23; break;
            case 15: state = (c == 'T') ? 22 : 23; break;

            /* Accepting states: any extra character → trap */
            case 16: case 17: case 18: case 19:
            case 20: case 21: case 22:
                state = 23;
                break;

            /* Trap: self-loop */
            case 23:
                state = 23;
                break;
        }
    }

    return (state >= 16 && state <= 22);
}

/* -----------------------------------------------------------------------
 * SemanticDFA  – faithful port of the Java state machine
 *
 *  States:
 *   1 = start            (expecting opcode)
 *   2 = accepting        nullary: HLT / NOP  (no operands)
 *   3 = unary path       expecting register  (INC / DEC)
 *   4 = accepting        unary done
 *   5 = binary path      expecting register-1  (MOV / ADD / SUB)
 *   6 = binary path      expecting comma
 *   7 = binary path      expecting register-2 or number
 *   8 = accepting        binary done
 *   9 = trap
 * ----------------------------------------------------------------------- */
int semantic_validate(const char *input) {
    Token tokens[MAX_TOKENS];
    int   count = tokenize(input, tokens);

    if (count == 0) return 0;

    int state = 1;

    for (int i = 0; i < count; i++) {
        Token *t = &tokens[i];

        switch (state) {
            case 1: /* Expecting OPCODE */
                if (t->type == OPCODE && opcode_validate(t->value)) {
                    /* Convert to uppercase for comparison */
                    char op[16];
                    for (int j = 0; t->value[j]; j++)
                        op[j] = (char)toupper((unsigned char)t->value[j]);
                    op[strlen(t->value)] = '\0';

                    if (strcmp(op, "HLT") == 0 || strcmp(op, "NOP") == 0)
                        state = 2;                  /* nullary path */
                    else if (strcmp(op, "INC") == 0 || strcmp(op, "DEC") == 0)
                        state = 3;                  /* unary path   */
                    else
                        state = 5;                  /* binary path  */
                } else {
                    state = 9;                      /* trap          */
                }
                break;

            /* NULLARY PATH */
            case 2:  /* HLT/NOP: any extra token → trap */
                state = 9;
                break;

            /* UNARY PATH */
            case 3:  /* Expecting register */
                state = (t->type == REGISTER && register_validate(t->value)) ? 4 : 9;
                break;
            case 4:  /* Done: extra token → trap */
                state = 9;
                break;

            /* BINARY PATH */
            case 5:  /* Expecting register-1 */
                state = (t->type == REGISTER && register_validate(t->value)) ? 6 : 9;
                break;
            case 6:  /* Expecting comma */
                state = (strcmp(t->value, ",") == 0) ? 7 : 9;
                break;
            case 7:  /* Expecting register-2 or number */
                if      (t->type == REGISTER && register_validate(t->value)) state = 8;
                else if (t->type == NUMBER   && number_validate(t->value))   state = 8;
                else                                                           state = 9;
                break;
            case 8:  /* Done: extra token → trap */
                state = 9;
                break;

            /* TRAP */
            case 9:
                state = 9;
                break;
        }
    }

    return (state == 2 || state == 4 || state == 8);
}