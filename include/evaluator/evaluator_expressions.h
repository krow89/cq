#ifndef EVALUATOR_EXPRESSIONS_H
#define EVALUATOR_EXPRESSIONS_H

#include "evaluator.h"
#include "parser.h"

/* expression evaluation */
Value evaluate_expression(QueryContext* ctx, ASTNode* expr, Row* current_row, int table_index);

#endif /* EVALUATOR_EXPRESSIONS_H */
