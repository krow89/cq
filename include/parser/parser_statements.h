#ifndef PARSER_STATEMENTS_H
#define PARSER_STATEMENTS_H

#include "parser.h"

/* DML statement parsing */
ASTNode* parse_insert(Parser* parser);
ASTNode* parse_update(Parser* parser);
ASTNode* parse_delete(Parser* parser);

/* DDL statement parsing */
ASTNode* parse_create_table(Parser* parser);
ASTNode* parse_alter_table(Parser* parser);

#endif /* PARSER_STATEMENTS_H */
