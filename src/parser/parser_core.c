#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "parser.h"
#include "tokenizer.h"
#include "parser/parser_core.h"

/* parser initialization and management functions */

Parser* parser_init(Token* tokens, int token_count) {
    Parser* parser = malloc(sizeof(Parser));
    parser->tokens = tokens;
    parser->token_count = token_count;
    parser->current_pos = 0;
    return parser;
}

void parser_free(Parser* parser) {
    free(parser);
}

Token* parser_current_token(Parser* parser) {
    if (parser->current_pos >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1]; // Return EOF
    }
    return &parser->tokens[parser->current_pos];
}

Token* parser_peek_token(Parser* parser, int offset) {
    int pos = parser->current_pos + offset;
    if (pos >= parser->token_count) {
        return &parser->tokens[parser->token_count - 1]; // Return EOF
    }
    return &parser->tokens[pos];
}

void parser_advance(Parser* parser) {
    if (parser->current_pos < parser->token_count - 1) {
        parser->current_pos++;
    }
}

int parser_match(Parser* parser, TokenType type, const char* value) {
    Token* token = parser_current_token(parser);
    if (token->type != type) {
        return 0;
    }
    if (value && strcasecmp(token->value, value) != 0) {
        return 0;
    }
    return 1;
}

int parser_expect(Parser* parser, TokenType type, const char* value) {
    if (!parser_match(parser, type, value)) {
        fprintf(stderr, "Parse error: expected %s but got %s\n", 
                value ? value : "token", parser_current_token(parser)->value);
        return 0;
    }
    parser_advance(parser);
    return 1;
}

/* helper: parse qualified identifier (e.g., table.column) */
char* parse_qualified_identifier(Parser* parser) {
    Token* token = parser_current_token(parser);
    if (token->type != TOKEN_TYPE_IDENTIFIER) {
        return NULL;
    }
    
    char buffer[256];
    int buf_pos = snprintf(buffer, sizeof(buffer), "%s", token->value);
    parser_advance(parser);
    
    // check for dot notation
    if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ".")) {
        parser_advance(parser); // skip '.'
        Token* col_token = parser_current_token(parser);
        if (col_token->type == TOKEN_TYPE_IDENTIFIER) {
            buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, ".%s", col_token->value);
            parser_advance(parser);
        }
    }
    
    return strdup(buffer);
}

/* helper: ensure dynamic array has enough capacity, realloc if needed */
void* ensure_capacity(void* array, int* capacity, int current_count, size_t element_size) {
    if (current_count >= *capacity) {
        *capacity *= 2;
        return realloc(array, element_size * (*capacity));
    }
    return array;
}

/* helper: parse table name or filename (handles quoted paths and .csv extensions) */
char* parse_table_name(Parser* parser) {
    Token* token = parser_current_token(parser);
    
    if (token->type == TOKEN_TYPE_LITERAL) {
        // quoted filename
        char* table = strdup(token->value);
        parser_advance(parser);
        return table;
    } else if (token->type == TOKEN_TYPE_IDENTIFIER) {
        // unquoted identifier with optional extension
        return parse_qualified_identifier(parser);
    }
    
    return NULL;
}

/* helper: parse optional alias (with or without AS keyword) */
char* parse_optional_alias(Parser* parser, const char** excluded_keywords, int excluded_count) {
    Token* token = parser_current_token(parser);
    
    // check for AS keyword
    if (parser_match(parser, TOKEN_TYPE_KEYWORD, "AS")) {
        parser_advance(parser);
        token = parser_current_token(parser);
        if (token->type == TOKEN_TYPE_IDENTIFIER) {
            char* alias = strdup(token->value);
            parser_advance(parser);
            return alias;
        }
        return NULL;
    }
    
    // check for alias without AS (but not if it's a SQL keyword)
    if (token->type == TOKEN_TYPE_IDENTIFIER) {
        for (int i = 0; i < excluded_count; i++) {
            if (strcasecmp(token->value, excluded_keywords[i]) == 0) {
                return NULL; // it's a SQL keyword, not an alias
            }
        }
        char* alias = strdup(token->value);
        parser_advance(parser);
        return alias;
    }
    
    return NULL;
}

/* helper to parse JOIN type keyword (LEFT/RIGHT/FULL/INNER with optional OUTER) */
JoinType parse_join_type(Parser* parser) {
    Token* token = parser_current_token(parser);
    
    if (token->type != TOKEN_TYPE_KEYWORD) {
        return JOIN_TYPE_INNER;
    }
    
    JoinType join_type = JOIN_TYPE_INNER;
    
    if (strcasecmp(token->value, "LEFT") == 0) {
        join_type = JOIN_TYPE_LEFT;
        parser_advance(parser);
    } else if (strcasecmp(token->value, "RIGHT") == 0) {
        join_type = JOIN_TYPE_RIGHT;
        parser_advance(parser);
    } else if (strcasecmp(token->value, "FULL") == 0) {
        join_type = JOIN_TYPE_FULL;
        parser_advance(parser);
    } else if (strcasecmp(token->value, "INNER") == 0) {
        join_type = JOIN_TYPE_INNER;
        parser_advance(parser);
    } else {
        return JOIN_TYPE_INNER; // default if no type specified
    }
    
    // check for optional OUTER keyword
    token = parser_current_token(parser);
    if (token->type == TOKEN_TYPE_KEYWORD && strcasecmp(token->value, "OUTER") == 0) {
        parser_advance(parser);
    }
    
    return join_type;
}

/* helper to build function call string for ORDER BY, legacy string-based approach */
char* build_function_string(Parser* parser) {
    Token* token = parser_current_token(parser);
    if (token->type != TOKEN_TYPE_IDENTIFIER) {
        return NULL;
    }
    
    Token* next = parser_peek_token(parser, 1);
    if (next->type != TOKEN_TYPE_PUNCTUATION || strcmp(next->value, "(") != 0) {
        return NULL;
    }
    
    char buffer[256];
    int buf_pos = snprintf(buffer, sizeof(buffer), "%s(", token->value);
    parser_advance(parser); // function name
    parser_advance(parser); // '('
    
    // capture function arguments
    bool first_arg = true;
    while (!parser_match(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
        Token* arg_token = parser_current_token(parser);
        
        if (strcmp(arg_token->value, ",") == 0) {
            buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, ", ");
            parser_advance(parser);
        } else if (arg_token->type == TOKEN_TYPE_IDENTIFIER) {
            if (!first_arg) {
                buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, ", ");
            }
            
            // build qualified identifier if needed
            buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, "%s", arg_token->value);
            parser_advance(parser);
            
            // check for dot notation
            if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ".")) {
                buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, ".");
                parser_advance(parser);
                Token* col_token = parser_current_token(parser);
                if (col_token->type == TOKEN_TYPE_IDENTIFIER) {
                    buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, "%s", col_token->value);
                    parser_advance(parser);
                }
            }
            
            first_arg = false;
        } else {
            // other tokens (literals, etc.)
            if (!first_arg) {
                buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, ", ");
            }
            buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, "%s", arg_token->value);
            parser_advance(parser);
            first_arg = false;
        }
    }
    buf_pos += snprintf(buffer + buf_pos, sizeof(buffer) - buf_pos, ")");
    parser_advance(parser); // ')'
    
    return strdup(buffer);
}
