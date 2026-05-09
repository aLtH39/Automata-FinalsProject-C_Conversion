#ifndef DFA_H
#define DFA_H

/* Each DFA mirrors the exact state machine from the Java source */
int register_validate(const char *input);
int number_validate(const char *input);
int opcode_validate(const char *input);
int semantic_validate(const char *input);

#endif