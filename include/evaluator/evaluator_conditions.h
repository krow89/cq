#ifndef EVALUATOR_CONDITIONS_H
#define EVALUATOR_CONDITIONS_H

#include "evaluator.h"
#include "parser.h"

/* condition evaluation */
bool evaluate_condition(QueryContext* ctx, ASTNode* condition, Row* current_row, int table_index);

#endif /* EVALUATOR_CONDITIONS_H */
