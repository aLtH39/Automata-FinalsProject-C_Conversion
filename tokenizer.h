#ifndef TOKENIZER_H
#define TOKENIZER_H

#define MAX_TOKEN_SIZE 64
#define MAX_TOKENS     32

typedef enum {
    OPCODE,
    REGISTER,
    NUMBER,
    UNKNOWN
} TokenType;

typedef struct {
    char      value[MAX_TOKEN_SIZE];
    TokenType type;
} Token;

/* Returns number of tokens produced */
int tokenize(const char *input, Token tokens[]);

/* Returns a static string like "[OPCODE: MOV]" */
const char *token_to_string(Token token);

/* Returns a hint string for an invalid instruction */
const char *hint(const char *line, Token tokens[], int count);

#endif