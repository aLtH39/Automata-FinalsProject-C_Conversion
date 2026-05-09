#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"
#include "dfa.h"

/* -----------------------------------------------------------------------
 * classify()
 * Mirrors Java Tokenizer.classify():
 *   1. Try RegisterDFA
 *   2. Try NumberDFA
 *   3. Try OpcodeDFA
 *   4. UNKNOWN (includes the comma token)
 * ----------------------------------------------------------------------- */
static TokenType classify(const char *token) {
    if (register_validate(token)) return REGISTER;
    if (number_validate(token))   return NUMBER;
    if (opcode_validate(token))   return OPCODE;
    return UNKNOWN;
}

/* -----------------------------------------------------------------------
 * tokenize()
 * Mirrors Java Tokenizer.tokenize():
 *   - Replace "," with " , " so the comma becomes its own token
 *   - Split on whitespace
 * ----------------------------------------------------------------------- */
int tokenize(const char *input, Token tokens[]) {
    /* Build a working copy with commas padded by spaces.
       Worst case: every char is a comma → 3× expansion + NUL */
    char buffer[1024];
    int  bi = 0;

    for (int i = 0; input[i] != '\0' && bi < (int)sizeof(buffer) - 4; i++) {
        if (input[i] == ',') {
            buffer[bi++] = ' ';
            buffer[bi++] = ',';
            buffer[bi++] = ' ';
        } else {
            buffer[bi++] = input[i];
        }
    }
    buffer[bi] = '\0';

    /* Tokenise on whitespace */
    int   count = 0;
    char *tok   = strtok(buffer, " \t\n\r");

    while (tok != NULL && count < MAX_TOKENS) {
        strncpy(tokens[count].value, tok, MAX_TOKEN_SIZE - 1);
        tokens[count].value[MAX_TOKEN_SIZE - 1] = '\0';
        tokens[count].type = classify(tok);
        count++;
        tok = strtok(NULL, " \t\n\r");
    }

    return count;
}

/* -----------------------------------------------------------------------
 * token_to_string()
 * Mirrors Java Token.toString(): "[TYPE: value]"
 * ----------------------------------------------------------------------- */
const char *token_to_string(Token token) {
    static char buffer[128];

    const char *type;
    switch (token.type) {
        case OPCODE:   type = "OPCODE";   break;
        case REGISTER: type = "REGISTER"; break;
        case NUMBER:   type = "NUMBER";   break;
        default:       type = "UNKNOWN";  break;
    }

    snprintf(buffer, sizeof(buffer), "[%s: %s]", type, token.value);
    return buffer;
}

/* -----------------------------------------------------------------------
 * hint()
 * Mirrors Java MAIN.hint(): friendly error message for invalid input.
 * ----------------------------------------------------------------------- */
const char *hint(const char *line, Token tokens[], int count) {
    (void)line; /* unused – kept for signature symmetry */

    static char buffer[256];

    if (count == 0) {
        snprintf(buffer, sizeof(buffer), "Empty input.");
        return buffer;
    }

    /* First token is not a recognised opcode */
    if (tokens[0].type == UNKNOWN) {
        snprintf(buffer, sizeof(buffer),
                 "\"%s\" is not a recognised opcode. "
                 "Valid opcodes: MOV, ADD, SUB, INC, DEC, NOP, HLT.",
                 tokens[0].value);
        return buffer;
    }

    /* Convert opcode to uppercase for the switch */
    char op[16];
    int  j;
    for (j = 0; tokens[0].value[j] && j < 15; j++)
        op[j] = (char)toupper((unsigned char)tokens[0].value[j]);
    op[j] = '\0';

    if (strcmp(op, "HLT") == 0 || strcmp(op, "NOP") == 0) {
        snprintf(buffer, sizeof(buffer),
                 "%s takes no operands. Example: %s", op, op);

    } else if (strcmp(op, "INC") == 0 || strcmp(op, "DEC") == 0) {
        snprintf(buffer, sizeof(buffer),
                 "%s takes one register. Example: %s R1", op, op);

    } else if (strcmp(op, "MOV") == 0 ||
               strcmp(op, "ADD") == 0 ||
               strcmp(op, "SUB") == 0) {
        snprintf(buffer, sizeof(buffer),
                 "%s takes a register, comma, then a register or number. "
                 "Example: %s R1, R2  or  %s R1, 42", op, op, op);

    } else {
        snprintf(buffer, sizeof(buffer), "Check your syntax and try again.");
    }

    return buffer;
}