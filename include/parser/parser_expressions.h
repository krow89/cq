#ifndef PARSER_EXPRESSIONS_H
#define PARSER_EXPRESSIONS_H

#include "parser.h"

/* expression parsing */
ASTNode* parse_expression(Parser* parser);
ASTNode* parse_condition(Parser* parser);
ASTNode* parse_primary(Parser* parser);

#endif /* PARSER_EXPRESSIONS_H */
