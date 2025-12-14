#ifndef EVALUATOR_FUNCTIONS_H
#define EVALUATOR_FUNCTIONS_H

#include "evaluator.h"

/* scalar function evaluation */
Value evaluate_scalar_function(const char* func_name, Value* args, int arg_count);

#endif /* EVALUATOR_FUNCTIONS_H */
