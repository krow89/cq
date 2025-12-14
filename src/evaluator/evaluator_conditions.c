#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "evaluator.h"
#include "parser.h"
#include "csv_reader.h"
#include "evaluator/evaluator_conditions.h"
#include "evaluator/evaluator_expressions.h"

// forward declarations
ResultSet* evaluate_query(ASTNode* query_ast);

// pattern matching helper for like/ilike operators
static bool match_pattern(const char* str, const char* pattern, bool case_sensitive) {
    if (!str || !pattern) return false;
    
    const char* s = str;
    const char* p = pattern;
    const char* star = NULL;
    const char* ss = NULL;
    
    while (*s) {
        if (*p == '%') {
            // remember position for backtracking
            star = p++;
            ss = s;
        } else if (*p == '_') {
            // single character wildcard
            s++;
            p++;
        } else {
            // check character match
            bool match;
            if (case_sensitive) {
                match = (*s == *p);
            } else {
                match = (tolower((unsigned char)*s) == tolower((unsigned char)*p));
            }
            
            if (match) {
                s++;
                p++;
            } else if (star) {
                // backtrack to last %
                p = star + 1;
                s = ++ss;
            } else {
                return false;
            }
        }
    }
    
    // consume remaining % at end of pattern
    while (*p == '%') p++;
    
    return *p == '\0';
}

// evaluate condition expressions, handles logical operators and comparisons
bool evaluate_condition(QueryContext* ctx, ASTNode* condition, Row* current_row, int table_index) {
    if (!condition) return true;
    
    if (condition->type != NODE_TYPE_CONDITION) return false;
    
    const char* op = condition->condition.operator;
    
    // handle unary NOT operator
    if (strcasecmp(op, "NOT") == 0) {
        bool result = evaluate_condition(ctx, condition->condition.left, current_row, table_index);
        return !result;
    }
    
    // handle logical operators
    if (strcasecmp(op, "AND") == 0) {
        bool left = evaluate_condition(ctx, condition->condition.left, current_row, table_index);
        bool right = evaluate_condition(ctx, condition->condition.right, current_row, table_index);
        return left && right;
    }
    
    if (strcasecmp(op, "OR") == 0) {
        bool left = evaluate_condition(ctx, condition->condition.left, current_row, table_index);
        bool right = evaluate_condition(ctx, condition->condition.right, current_row, table_index);
        return left || right;
    }
    
    // handle comparison operators
    Value left = evaluate_expression(ctx, condition->condition.left, current_row, table_index);
    Value right = evaluate_expression(ctx, condition->condition.right, current_row, table_index);
    
    int cmp = value_compare(&left, &right);
    
    if (strcmp(op, "=") == 0) return cmp == 0;
    if (strcmp(op, "!=") == 0) return cmp != 0;
    if (strcmp(op, "<>") == 0) return cmp != 0;
    if (strcmp(op, ">") == 0) return cmp > 0;
    if (strcmp(op, "<") == 0) return cmp < 0;
    if (strcmp(op, ">=") == 0) return cmp >= 0;
    if (strcmp(op, "<=") == 0) return cmp <= 0;
    
    // handle IN and NOT IN operators
    if (strcasecmp(op, "IN") == 0 || strcasecmp(op, "NOT IN") == 0) {
        bool is_not_in = (strcasecmp(op, "NOT IN") == 0);
        ASTNode* right_node = condition->condition.right;
        
        // check if it's a subquery or a list
        if (right_node->type == NODE_TYPE_SUBQUERY) {
            // evaluate the subquery
            if (!right_node->subquery.query) return is_not_in; // NOT IN empty = true
            
            ResultSet* subquery_result = evaluate_query(right_node->subquery.query);
            if (!subquery_result) return is_not_in;
            
            // the subquery should return a single column
            if (subquery_result->column_count != 1) {
                fprintf(stderr, "Error: IN subquery must return exactly one column\n");
                csv_free(subquery_result);
                return false;
            }
            
            // check if left value matches any value in the subquery result
            bool found = false;
            for (int i = 0; i < subquery_result->row_count; i++) {
                Value* subquery_val = &subquery_result->rows[i].values[0];
                if (value_compare(&left, subquery_val) == 0) {
                    found = true;
                    break;
                }
            }
            
            csv_free(subquery_result);
            return is_not_in ? !found : found;
        } else if (right_node->type == NODE_TYPE_LIST) {
            // list-based IN operator
            ASTNode* list = right_node;
            for (int i = 0; i < list->list.node_count; i++) {
                Value list_val = evaluate_expression(ctx, list->list.nodes[i], current_row, table_index);
                if (value_compare(&left, &list_val) == 0) {
                    value_free(&list_val);
                    return is_not_in ? false : true;
                }
                value_free(&list_val);
            }
            return is_not_in ? true : false;
        }
        
        return is_not_in; // empty list: NOT IN = true, IN = false
    }
    
    // handle LIKE and ILIKE operators
    if (strcasecmp(op, "LIKE") == 0 || strcasecmp(op, "ILIKE") == 0) {
        bool case_sensitive = (strcasecmp(op, "LIKE") == 0);
        
        // both operands must be strings
        if (left.type != VALUE_TYPE_STRING || right.type != VALUE_TYPE_STRING) {
            return false;
        }
        
        return match_pattern(left.string_value, right.string_value, case_sensitive);
    }
    
    return false;
}
