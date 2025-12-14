#ifndef PARSER_CORE_H
#define PARSER_CORE_H

#include "parser.h"
#include "tokenizer.h"

/* parser initialization and cleanup */
Parser* parser_init(Token* tokens, int token_count);
void parser_free(Parser* parser);

/* token navigation */
Token* parser_current_token(Parser* parser);
Token* parser_peek_token(Parser* parser, int offset);
void parser_advance(Parser* parser);

/* token matching and validation */
int parser_match(Parser* parser, TokenType type, const char* value);
int parser_expect(Parser* parser, TokenType type, const char* value);

/* utility functions */
void* ensure_capacity(void* array, int* capacity, int current_count, size_t element_size);

#endif /* PARSER_CORE_H */
