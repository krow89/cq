#ifndef TOKENIZER_H
#define TOKENIZER_H

/* token types */
typedef enum {
    TOKEN_TYPE_KEYWORD,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_LITERAL,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_PUNCTUATION,
    TOKEN_TYPE_EOF,
} TokenType;

typedef struct {
    int refcount;
    TokenType type;
    char* value;
} Token;


/* as we will use reference counting for memory management, we need utility functions to handle it */
void retainToken(Token* token);
void releaseToken(Token* token);

Token* tokenize(const char* sql, int* token_count);

/* utility functions for debugging and memory management */
void printTokens(Token* tokens, int token_count);
void freeTokens(Token* tokens, int token_count);

#endif