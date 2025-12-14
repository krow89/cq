#ifndef EVALUATOR_WINDOW_H
#define EVALUATOR_WINDOW_H

#include "evaluator.h"
#include "parser.h"

/* window function evaluation */
Value* evaluate_window_function(ASTNode* win_func, QueryContext* ctx, Row** rows, int row_count);

#endif /* EVALUATOR_WINDOW_H */
