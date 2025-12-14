#ifndef PARSER_CLAUSES_H
#define PARSER_CLAUSES_H

#include "parser.h"

/* clause parsing */
ASTNode* parse_select(Parser* parser);
ASTNode* parse_from(Parser* parser);
ASTNode* parse_where(Parser* parser);
ASTNode* parse_group_by(Parser* parser);
ASTNode* parse_order_by(Parser* parser);
ASTNode* parse_join(Parser* parser);

#endif /* PARSER_CLAUSES_H */
