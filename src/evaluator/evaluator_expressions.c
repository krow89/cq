/*
 * evaluator_expressions.c
 * expression evaluation functions for the query evaluator
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include "evaluator.h"
#include "parser.h"
#include "csv_reader.h"
#include "string_utils.h"
#include "evaluator/evaluator_expressions.h"
#include "evaluator/evaluator_core.h"
#include "evaluator/evaluator_functions.h"
#include "evaluator/evaluator_conditions.h"
#include "evaluator/evaluator_utils.h"
#include "evaluator/evaluator_internal.h"

Value evaluate_expression(QueryContext* ctx, ASTNode* expr, Row* current_row, int table_index) {
    Value result;
    result.type = VALUE_TYPE_NULL;
    
    if (!expr) return result;
    
    switch (expr->type) {
        case NODE_TYPE_LITERAL:
            return parse_value(expr->literal, strlen(expr->literal));
            
        case NODE_TYPE_IDENTIFIER: {
            // resolve column value
            Value* val = resolve_column(ctx, expr->identifier, current_row, table_index);
            if (val) {
                // do a deep copy to avoid freeing shared string pointers
                value_deep_copy(&result, val);
                return result;
            }
            break;
        }
        
        case NODE_TYPE_SUBQUERY: {
            // evaluate scalar subquery, that may be correlated
            // Note: This is for scalar contexts (SELECT, comparison ops)
            // IN operator handles multi-row subqueries separately in evaluate_condition
            if (!expr->subquery.query) break;
            
            // pass the outer context for correlated subqueries
            ResultSet* subquery_result = evaluate_query_internal(expr->subquery.query, 
                current_row, ctx->tables[table_index].table);
            
            if (!subquery_result) break;
            
            // validation only in scalar context (not for IN operator)
            // IN operator is handled separately and allows multi-row results
            if (subquery_result->row_count != 1 || subquery_result->column_count != 1) {
                // only report error if it's truly being used as scalar
                // don't report for IN operator which is handled elsewhere
                csv_free(subquery_result);
                break;
            }
            
            // copy the single value from subquery result
            value_deep_copy(&result, &subquery_result->rows[0].values[0]);
            csv_free(subquery_result);
            return result;
        }
        
        case NODE_TYPE_FUNCTION: {
            // evaluate function call in WHERE clause
            Value func_args[10];
            int func_arg_count = 0;
            
            // evaluate each argument recursively
            for (int i = 0; i < expr->function.arg_count && i < 10; i++) {
                func_args[func_arg_count++] = evaluate_expression(ctx, expr->function.args[i], current_row, table_index);
            }
            
            // call the function
            result = evaluate_scalar_function(expr->function.name, func_args, func_arg_count);
            
            // free temporary string values allocated during argument evaluation
            for (int i = 0; i < func_arg_count; i++) {
                if (func_args[i].type == VALUE_TYPE_STRING && func_args[i].string_value) {
                    free((char*)func_args[i].string_value);
                }
            }
            
            return result;
        }
        
        case NODE_TYPE_WINDOW_FUNCTION:
            // window functions cannot be evaluated in regular expression context
            // they are handled separately during result building
            fprintf(stderr, "Error: Window functions can only be used in SELECT clause\n");
            result.type = VALUE_TYPE_NULL;
            return result;
        
        case NODE_TYPE_BINARY_OP: {
            // check for unary operator (left is NULL, only right exists)
            if (!expr->binary_op.left) {
                if (!expr->binary_op.right) {
                    result.type = VALUE_TYPE_NULL;
                    return result;
                }
                
                const char* op = expr->binary_op.operator;
                Value operand = evaluate_expression(ctx, expr->binary_op.right, current_row, table_index);
                
                if (strcmp(op, "-") == 0) {
                    // unary minus
                    if (operand.type == VALUE_TYPE_INTEGER) {
                        result.type = VALUE_TYPE_INTEGER;
                        result.int_value = -operand.int_value;
                        return result;
                    } else if (operand.type == VALUE_TYPE_DOUBLE) {
                        result.type = VALUE_TYPE_DOUBLE;
                        result.double_value = -operand.double_value;
                        return result;
                    }
                } else if (strcmp(op, "+") == 0) {
                    // unary plus (no-op)
                    return operand;
                }
                result.type = VALUE_TYPE_NULL;
                return result;
            }
            
            // evaluate binary arithmetic operation
            Value left = evaluate_expression(ctx, expr->binary_op.left, current_row, table_index);
            
            // check for unary operator (right is NULL), shouldn't happen with current parser
            if (!expr->binary_op.right) {
                const char* op = expr->binary_op.operator;
                if (strcmp(op, "-") == 0) {
                    // unary minus
                    if (left.type == VALUE_TYPE_INTEGER) {
                        result.type = VALUE_TYPE_INTEGER;
                        result.int_value = -left.int_value;
                        return result;
                    } else if (left.type == VALUE_TYPE_DOUBLE) {
                        result.type = VALUE_TYPE_DOUBLE;
                        result.double_value = -left.double_value;
                        return result;
                    }
                } else if (strcmp(op, "+") == 0) {
                    // unary plus (no-op)
                    return left;
                }
                result.type = VALUE_TYPE_NULL;
                return result;
            }
            
            Value right = evaluate_expression(ctx, expr->binary_op.right, current_row, table_index);
            const char* op = expr->binary_op.operator;
            
            // convert to numeric values
            double left_val = 0, right_val = 0;
            long long left_int = 0, right_int = 0;
            bool left_is_int = false, right_is_int = false;
            
            if (left.type == VALUE_TYPE_INTEGER) {
                left_val = (double)left.int_value;
                left_int = left.int_value;
                left_is_int = true;
            } else if (left.type == VALUE_TYPE_DOUBLE) {
                left_val = left.double_value;
            } else {
                result.type = VALUE_TYPE_NULL;
                return result;
            }
            
            if (right.type == VALUE_TYPE_INTEGER) {
                right_val = (double)right.int_value;
                right_int = right.int_value;
                right_is_int = true;
            } else if (right.type == VALUE_TYPE_DOUBLE) {
                right_val = right.double_value;
            } else {
                result.type = VALUE_TYPE_NULL;
                return result;
            }
            
            // perform the operation
            double result_val = 0;
            long long result_int = 0;
            bool result_is_int = false;
            
            if (strcmp(op, "+") == 0) {
                result_val = left_val + right_val;
            } else if (strcmp(op, "-") == 0) {
                result_val = left_val - right_val;
            } else if (strcmp(op, "*") == 0) {
                result_val = left_val * right_val;
            } else if (strcmp(op, "/") == 0) {
                if (right_val == 0) {
                    result.type = VALUE_TYPE_NULL;
                    return result;
                }
                result_val = left_val / right_val;
            } else if (strcmp(op, "%") == 0) {
                // % requires integers
                if (left_is_int && right_is_int) {
                    if (right_int == 0) {
                        result.type = VALUE_TYPE_NULL;
                        return result;
                    }
                    result_int = left_int % right_int;
                    result_is_int = true;
                } else {
                    // doubles use fmod
                    if (right_val == 0) {
                        result.type = VALUE_TYPE_NULL;
                        return result;
                    }
                    result_val = fmod(left_val, right_val);
                }
            } else if (strcmp(op, "&") == 0) {
                // bitwise AND requires integers
                if (left_is_int && right_is_int) {
                    result_int = left_int & right_int;
                    result_is_int = true;
                } else {
                    result.type = VALUE_TYPE_NULL;
                    return result;
                }
            } else if (strcmp(op, "|") == 0) {
                // bitwise OR requires integers
                if (left_is_int && right_is_int) {
                    result_int = left_int | right_int;
                    result_is_int = true;
                } else {
                    result.type = VALUE_TYPE_NULL;
                    return result;
                }
            } else if (strcmp(op, "^") == 0) {
                // bitwise XOR requires integers
                if (left_is_int && right_is_int) {
                    result_int = left_int ^ right_int;
                    result_is_int = true;
                } else {
                    result.type = VALUE_TYPE_NULL;
                    return result;
                }
            }
            
            // return result
            if (result_is_int) {
                result.type = VALUE_TYPE_INTEGER;
                result.int_value = result_int;
            } else if (left.type == VALUE_TYPE_INTEGER && right.type == VALUE_TYPE_INTEGER && 
                       result_val == (long long)result_val) {
                result.type = VALUE_TYPE_INTEGER;
                result.int_value = (long long)result_val;
            } else {
                result.type = VALUE_TYPE_DOUBLE;
                result.double_value = result_val;
            }
            
            return result;
        }
            
        case NODE_TYPE_CASE: {
            // evaluate CASE expression
            if (!expr->case_expr.when_exprs || !expr->case_expr.then_exprs) {
                result.type = VALUE_TYPE_NULL;
                return result;
            }
            
            // check if this is simple CASE or searched CASE
            bool is_simple_case = (expr->case_expr.case_expr != NULL);
            Value case_value;
            
            if (is_simple_case) {
                // simple CASE: evaluate the expression after CASE
                case_value = evaluate_expression(ctx, expr->case_expr.case_expr, current_row, table_index);
            }
            
            // iterate through WHEN/THEN pairs
            for (int i = 0; i < expr->case_expr.when_count; i++) {
                bool when_matches = false;
                
                if (is_simple_case) {
                    // simple CASE: compare case_value with each WHEN value
                    Value when_value = evaluate_expression(ctx, expr->case_expr.when_exprs[i], current_row, table_index);
                    when_matches = (value_compare(&case_value, &when_value) == 0);
                    
                    // free string values if needed
                    if (when_value.type == VALUE_TYPE_STRING && when_value.string_value) {
                        free((char*)when_value.string_value);
                    }
                } else {
                    // searched CASE: WHEN clause is a condition node (age > 30, etc.)
                    when_matches = evaluate_condition(ctx, expr->case_expr.when_exprs[i], current_row, table_index);
                }
                
                if (when_matches) {
                    // evaluate and return the corresponding THEN expression
                    result = evaluate_expression(ctx, expr->case_expr.then_exprs[i], current_row, table_index);
                    
                    // free case_value string if needed
                    if (is_simple_case && case_value.type == VALUE_TYPE_STRING && case_value.string_value) {
                        free((char*)case_value.string_value);
                    }
                    return result;
                }
            }
            
            // no WHEN matched, evaluate ELSE (or return NULL if no ELSE)
            if (expr->case_expr.else_expr) {
                result = evaluate_expression(ctx, expr->case_expr.else_expr, current_row, table_index);
            } else {
                result.type = VALUE_TYPE_NULL;
            }
            
            // free case_value string if needed
            if (is_simple_case && case_value.type == VALUE_TYPE_STRING && case_value.string_value) {
                free((char*)case_value.string_value);
            }
            return result;
        }
            
        default:
            break;
    }
    
    return result;
}
