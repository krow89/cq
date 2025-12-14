#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "parser.h"
#include "tokenizer.h"
#include "parser/parser_expressions.h"
#include "parser/ast_nodes.h"
#include "parser/parser_core.h"
#include "parser/parser_internal.h"

/* forward declarations */
static ASTNode* parse_additive_expr(Parser* parser);
static ASTNode* parse_logical_continuation(Parser* parser, ASTNode* left_condition);

static ASTNode* parse_logical_continuation(Parser* parser, ASTNode* left_condition) {
    Token* token = parser_current_token(parser);
    if (token->type == TOKEN_TYPE_KEYWORD && 
        (strcasecmp(token->value, "AND") == 0 || strcasecmp(token->value, "OR") == 0)) {
        char* logical_op = token->value;
        parser_advance(parser);
        ASTNode* right = parse_condition(parser);
        return create_condition_node(left_condition, logical_op, right);
    }
    return left_condition;
}

ASTNode* parse_function_call(Parser* parser, bool allow_distinct) {
    Token* token = parser_current_token(parser);
    Token* next = parser_peek_token(parser, 1);
    
    // allow both IDENTIFIER and KEYWORD for function names (window functions are keywords)
    if ((token->type != TOKEN_TYPE_IDENTIFIER && token->type != TOKEN_TYPE_KEYWORD) || !next || 
        next->type != TOKEN_TYPE_PUNCTUATION || strcmp(next->value, "(") != 0) {
        return NULL;
    }
    
    char* func_name = strdup(token->value);
    parser_advance(parser); // skip function name
    parser_advance(parser); // skip (
    
    // collect function arguments
    int capacity = 4;
    ASTNode** args = malloc(sizeof(ASTNode*) * capacity);
    int arg_count = 0;
    
    // handle empty function calls like NOW()
    if (!parser_match(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
        // parse DISTINCT if allowed
        if (allow_distinct && parser_match(parser, TOKEN_TYPE_KEYWORD, "DISTINCT")) {
            parser_advance(parser);
        }
    
    // parse arguments if not empty
    
    while (!parser_match(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
        if (arg_count >= capacity) {
            capacity *= 2;
            args = realloc(args, sizeof(ASTNode*) * capacity);
        }
        
        // handle COUNT(*) or other aggregate functions with *
        Token* current = parser_current_token(parser);
        if (current->type == TOKEN_TYPE_OPERATOR && strcmp(current->value, "*") == 0) {
            args[arg_count++] = create_literal_node("*");
            parser_advance(parser);
            
            // no more arguments after *
            if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                parser_advance(parser);
            } else {
                break;
            }
        } else {
            // parse a regular argument which could be a full expression
            ASTNode* arg = parse_expression(parser);
            if (!arg) {
                fprintf(stderr, "Parse error: Invalid function argument\n");
                for (int i = 0; i < arg_count; i++) {
                    releaseNode(args[i]);
                }
                free(args);
                free(func_name);
                return NULL;
            }
            args[arg_count++] = arg;
            
            if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                parser_advance(parser);
            }
        }
        
        if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
            parser_advance(parser);
        }
    }
    }  // end of if (!parser_match(...
    parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
    
    // check for OVER clause (window function)
    if (parser_match(parser, TOKEN_TYPE_KEYWORD, "OVER")) {
        parser_advance(parser); // skip OVER
        parser_expect(parser, TOKEN_TYPE_PUNCTUATION, "(");
        
        ASTNode* node = create_node(NODE_TYPE_WINDOW_FUNCTION);
        node->window_function.name = func_name;
        node->window_function.args = args;
        node->window_function.arg_count = arg_count;
        node->window_function.partition_by = NULL;
        node->window_function.partition_count = 0;
        node->window_function.order_by_column = NULL;
        node->window_function.order_descending = false;
        
        // parse PARTITION BY (optional)
        if (parser_match(parser, TOKEN_TYPE_KEYWORD, "PARTITION")) {
            parser_advance(parser); // skip PARTITION
            parser_expect(parser, TOKEN_TYPE_KEYWORD, "BY");
            
            int part_capacity = 4;
            node->window_function.partition_by = malloc(sizeof(char*) * part_capacity);
            
            while (1) {
                Token* col = parser_current_token(parser);
                if (col->type != TOKEN_TYPE_IDENTIFIER) {
                    fprintf(stderr, "Error: Expected column name after PARTITION BY\n");
                    releaseNode(node);
                    return NULL;
                }
                
                node->window_function.partition_by = ensure_capacity(
                    node->window_function.partition_by, &part_capacity,
                    node->window_function.partition_count, sizeof(char*)
                );
                node->window_function.partition_by[node->window_function.partition_count++] = strdup(col->value);
                parser_advance(parser);
                
                if (!parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                    break;
                }
                parser_advance(parser); // skip comma
            }
        }
        
        // parse ORDER BY (optional)
        if (parser_match(parser, TOKEN_TYPE_KEYWORD, "ORDER")) {
            parser_advance(parser); // skip ORDER
            parser_expect(parser, TOKEN_TYPE_KEYWORD, "BY");
            
            Token* col = parser_current_token(parser);
            if (col->type != TOKEN_TYPE_IDENTIFIER) {
                fprintf(stderr, "Error: Expected column name after ORDER BY\n");
                releaseNode(node);
                return NULL;
            }
            
            node->window_function.order_by_column = strdup(col->value);
            parser_advance(parser);
            
            // check for DESC/ASC
            if (parser_match(parser, TOKEN_TYPE_KEYWORD, "DESC")) {
                node->window_function.order_descending = true;
                parser_advance(parser);
            } else if (parser_match(parser, TOKEN_TYPE_KEYWORD, "ASC")) {
                parser_advance(parser);
            }
        }
        
        parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
        return node;
    }
    
    // regular function (not a window function)
    ASTNode* node = create_node(NODE_TYPE_FUNCTION);
    node->function.name = func_name;
    node->function.args = args;
    node->function.arg_count = arg_count;
    return node;
}

/* parse CASE expression, it handles both simple and searched CASE */
static ASTNode* parse_case(Parser* parser) {
    if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "CASE")) {
        return NULL;
    }
    parser_advance(parser); // skip CASE
    
    ASTNode* node = create_node(NODE_TYPE_CASE);
    node->case_expr.case_expr = NULL;
    node->case_expr.when_exprs = NULL;
    node->case_expr.then_exprs = NULL;
    node->case_expr.when_count = 0;
    node->case_expr.else_expr = NULL;
    
    // check if this is simple CASE (has an expression after CASE)
    // or searched CASE (goes directly to WHEN)
    Token* next = parser_current_token(parser);
    if (next->type != TOKEN_TYPE_KEYWORD || strcasecmp(next->value, "WHEN") != 0) {
        // simple CASE: CASE expr WHEN value THEN result ...
        node->case_expr.case_expr = parse_expression(parser);
    }
    
    // parse WHEN...THEN clauses
    int capacity = 4;
    node->case_expr.when_exprs = malloc(sizeof(ASTNode*) * capacity);
    node->case_expr.then_exprs = malloc(sizeof(ASTNode*) * capacity);
    
    while (parser_match(parser, TOKEN_TYPE_KEYWORD, "WHEN")) {
        parser_advance(parser); // skip WHEN
        
        // ensure capacity
        if (node->case_expr.when_count >= capacity) {
            capacity *= 2;
            node->case_expr.when_exprs = realloc(node->case_expr.when_exprs, sizeof(ASTNode*) * capacity);
            node->case_expr.then_exprs = realloc(node->case_expr.then_exprs, sizeof(ASTNode*) * capacity);
        }
        
        // parse WHEN condition/value
        // for simple CASE, this is a value to compare, for searched CASE, this is a condition
        if (node->case_expr.case_expr) {
            // simple CASE, parse as expression (value to compare)
            node->case_expr.when_exprs[node->case_expr.when_count] = parse_expression(parser);
        } else {
            // searched CASE, parse as condition (boolean expression)
            node->case_expr.when_exprs[node->case_expr.when_count] = parse_condition(parser);
        }
        
        // expect THEN
        if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "THEN")) {
            fprintf(stderr, "Parse error: Expected THEN after WHEN condition\n");
            releaseNode(node);
            return NULL;
        }
        parser_advance(parser); // skip THEN
        
        // parse THEN result
        node->case_expr.then_exprs[node->case_expr.when_count] = parse_expression(parser);
        node->case_expr.when_count++;
    }
    
    // check for ELSE clause
    if (parser_match(parser, TOKEN_TYPE_KEYWORD, "ELSE")) {
        parser_advance(parser); // skip ELSE
        node->case_expr.else_expr = parse_expression(parser);
    }
    
    // expect END
    if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "END")) {
        fprintf(stderr, "Parse error: Expected END to close CASE expression\n");
        releaseNode(node);
        return NULL;
    }
    parser_advance(parser); // skip END
    
    return node;
}

/* function for parsing the primary expression */
ASTNode* parse_primary(Parser* parser) {
    Token* token = parser_current_token(parser);
    
    // handle CASE expressions
    if (token->type == TOKEN_TYPE_KEYWORD && strcasecmp(token->value, "CASE") == 0) {
        return parse_case(parser);
    }
    
    // handle parentheses, it could be a subquery or expression
    if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, "(")) {
        parser_advance(parser);
        
        // check if this is a scalar subquery, it starts with SELECT
        Token* next = parser_current_token(parser);
        if (next->type == TOKEN_TYPE_KEYWORD && strcasecmp(next->value, "SELECT") == 0) {
            // parse as scalar subquery
            ASTNode* subquery_node = create_node(NODE_TYPE_SUBQUERY);
            subquery_node->subquery.query = parse_query_internal(parser);
            parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
            return subquery_node;
        }
        
        // otherwise, parse as regular expression/condition
        ASTNode* expr = parse_condition(parser);
        parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
        return expr;
    }
    
    // handle function calls
    ASTNode* func = parse_function_call(parser, true);
    if (func) return func;
    
    // handle identifiers including qualified identifiers like table.column
    if (token->type == TOKEN_TYPE_IDENTIFIER) {
        char* identifier = parse_qualified_identifier(parser);
        if (identifier) {
            return create_identifier_node(identifier);
        }
    }
    
    // handle literals
    if (token->type == TOKEN_TYPE_LITERAL) {
        parser_advance(parser);
        return create_literal_node(token->value);
    }
    
    // handle * for COUNT(*)
    if (token->type == TOKEN_TYPE_OPERATOR && strcmp(token->value, "*") == 0) {
        parser_advance(parser);
        return create_literal_node("*");
    }
    
    return NULL;
}

/* forward declarations for recursive descent parser */
static ASTNode* parse_bitwise_expr(Parser* parser);
static ASTNode* parse_multiplicative_expr(Parser* parser);
static ASTNode* parse_arithmetic_primary(Parser* parser);

/* parse term handling *, /, % */
static ASTNode* parse_multiplicative_expr(Parser* parser) {
    ASTNode* left = parse_arithmetic_primary(parser);
    
    while (1) {
        Token* token = parser_current_token(parser);
        if (token->type == TOKEN_TYPE_OPERATOR && 
            (strcmp(token->value, "*") == 0 || strcmp(token->value, "/") == 0 || strcmp(token->value, "%") == 0)) {
            char* op = token->value;
            parser_advance(parser);
            ASTNode* right = parse_arithmetic_primary(parser);
            left = create_binary_op_node(left, op, right);
        } else {
            break;
        }
    }
    
    return left;
}

/* parse expression handling +, - */
static ASTNode* parse_additive_expr(Parser* parser) {
    ASTNode* left = parse_multiplicative_expr(parser);
    
    while (1) {
        Token* token = parser_current_token(parser);
        if (token->type == TOKEN_TYPE_OPERATOR && 
            (strcmp(token->value, "+") == 0 || strcmp(token->value, "-") == 0)) {
            char* op = token->value;
            parser_advance(parser);
            ASTNode* right = parse_multiplicative_expr(parser);
            left = create_binary_op_node(left, op, right);
        } else {
            break;
        }
    }
    
    return left;
}

/* parse bitwise operations handling &, | and ^ with lowest precedence for arithmetic */
static ASTNode* parse_bitwise_expr(Parser* parser) {
    ASTNode* left = parse_additive_expr(parser);
    
    while (1) {
        Token* token = parser_current_token(parser);
        if (token->type == TOKEN_TYPE_OPERATOR && 
            (strcmp(token->value, "&") == 0 || strcmp(token->value, "|") == 0 || strcmp(token->value, "^") == 0)) {
            char* op = token->value;
            parser_advance(parser);
            ASTNode* right = parse_additive_expr(parser);
            left = create_binary_op_node(left, op, right);
        } else {
            break;
        }
    }
    
    return left;
}

/* parse primary arithmetic expression for number, identifier, function call, or parenthesized expression */
static ASTNode* parse_arithmetic_primary(Parser* parser) {
    Token* token = parser_current_token(parser);
    
    // handle CASE expressions
    if (token->type == TOKEN_TYPE_KEYWORD && strcasecmp(token->value, "CASE") == 0) {
        return parse_case(parser);
    }
    
    // handle unary operators (- and +)
    if (token->type == TOKEN_TYPE_OPERATOR && 
        (strcmp(token->value, "-") == 0 || strcmp(token->value, "+") == 0)) {
        char* op = token->value;
        parser_advance(parser);
        ASTNode* operand = parse_arithmetic_primary(parser);
        // create a binary_op node with NULL left to indicate unary operator
        return create_binary_op_node(NULL, op, operand);
    }
    
    // handle parentheses
    if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, "(")) {
        parser_advance(parser);
        
        // check if this is a scalar subquery
        Token* next = parser_current_token(parser);
        if (next->type == TOKEN_TYPE_KEYWORD && strcasecmp(next->value, "SELECT") == 0) {
            ASTNode* subquery_node = create_node(NODE_TYPE_SUBQUERY);
            subquery_node->subquery.query = parse_query_internal(parser);
            parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
            return subquery_node;
        }
        
        // otherwise parse as arithmetic expression respecting all precedence levels
        ASTNode* expr = parse_bitwise_expr(parser);
        parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
        return expr;
    }
    
    // handle function calls
    ASTNode* func = parse_function_call(parser, false);
    if (func) return func;
    
    // handle identifiers including qualified identifiers like table.column
    if (token->type == TOKEN_TYPE_IDENTIFIER) {
        char* identifier = parse_qualified_identifier(parser);
        if (identifier) {
            return create_identifier_node(identifier);
        }
    }
    
    // handle literals
    if (token->type == TOKEN_TYPE_LITERAL) {
        parser_advance(parser);
        return create_literal_node(token->value);
    }
    
    return NULL;
}

/* parse arithmetic expression, entry point for expressions
 * start with bitwise to include all precedence levels */
ASTNode* parse_expression(Parser* parser) {
    return parse_bitwise_expr(parser);
}

/* condition parsing for WHERE clause */
ASTNode* parse_condition(Parser* parser) {
    // handle NOT operator
    if (parser_match(parser, TOKEN_TYPE_KEYWORD, "NOT")) {
        parser_advance(parser);
        
        // check if followed by parentheses for grouped condition
        if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, "(")) {
            parser_advance(parser);
            ASTNode* condition = parse_condition(parser);
            parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
            return create_condition_node(condition, "NOT", NULL);
        }
        
        ASTNode* condition = parse_condition(parser);
        // create a NOT condition node using "NOT" as operator with NULL right side
        return create_condition_node(condition, "NOT", NULL);
    }
    
    // parse left side that can be arithmetic expression with parentheses
    ASTNode* left = parse_expression(parser);
    
    // check for comparison operator
    Token* token = parser_current_token(parser);
    
    // check for NOT IN pattern
    bool is_not_in = false;
    if (token->type == TOKEN_TYPE_KEYWORD && strcasecmp(token->value, "NOT") == 0) {
        Token* next = parser_peek_token(parser, 1);
        if (next->type == TOKEN_TYPE_KEYWORD && strcasecmp(next->value, "IN") == 0) {
            is_not_in = true;
            parser_advance(parser); // skip NOT
            token = parser_current_token(parser); // now on IN
        }
    }
    
    // check for BETWEEN operator
    if (token->type == TOKEN_TYPE_KEYWORD && strcasecmp(token->value, "BETWEEN") == 0) {
        parser_advance(parser);
        
        // parse lower bound
        ASTNode* lower = parse_expression(parser);
        
        // expect AND keyword
        if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "AND")) {
            releaseNode(left);
            releaseNode(lower);
            return NULL;
        }
        
        // parse upper bound
        ASTNode* upper = parse_expression(parser);
        
        // create condition, left >= lower AND left <= upper
        // we need to duplicate left for the second comparison
        ASTNode* left_copy = NULL;
        if (left->type == NODE_TYPE_IDENTIFIER) {
            left_copy = create_identifier_node(left->identifier);
        } else if (left->type == NODE_TYPE_LITERAL) {
            left_copy = create_literal_node(left->literal);
        } else if (left->type == NODE_TYPE_BINARY_OP) {
            // for complex expressions, just reuse with incremented refcount
            left_copy = left;
            retainNode(left_copy);
        } else if (left->type == NODE_TYPE_FUNCTION) {
            // for function calls, reuse with incremented refcount
            left_copy = left;
            retainNode(left_copy);
        } else {
            // fallback, reuse node with incremented refcount
            left_copy = left;
            retainNode(left_copy);
        }
        
        ASTNode* cond_lower = create_condition_node(left, ">=", lower);
        ASTNode* cond_upper = create_condition_node(left_copy, "<=", upper);
        ASTNode* condition = create_condition_node(cond_lower, "AND", cond_upper);
        
        return parse_logical_continuation(parser, condition);
    }
    
    if (token->type == TOKEN_TYPE_OPERATOR || 
        (token->type == TOKEN_TYPE_KEYWORD && 
         (strcasecmp(token->value, "IN") == 0 || 
          strcasecmp(token->value, "LIKE") == 0 || 
          strcasecmp(token->value, "ILIKE") == 0))) {
        char* op = is_not_in ? "NOT IN" : token->value;
        parser_advance(parser);
        
        // handle IN clause with list or subquery
        if ((strcasecmp(token->value, "IN") == 0) && parser_match(parser, TOKEN_TYPE_PUNCTUATION, "(")) {
            parser_advance(parser);
            
            // check if it's a subquery, it starts with SELECT
            Token* peek = parser_current_token(parser);
            if (peek->type == TOKEN_TYPE_KEYWORD && strcasecmp(peek->value, "SELECT") == 0) {
                // it's a subquery
                ASTNode* subquery_query = parse_query_internal(parser);
                if (!subquery_query) {
                    releaseNode(left);
                    return NULL;
                }
                
                if (!parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
                    releaseNode(subquery_query);
                    releaseNode(left);
                    return NULL;
                }
                
                // create subquery node
                ASTNode* subquery_node = create_node(NODE_TYPE_SUBQUERY);
                subquery_node->subquery.query = subquery_query;
                
                ASTNode* condition = create_condition_node(left, op, subquery_node);
                return parse_logical_continuation(parser, condition);
            } else {
                // it's a list of values
                ASTNode* list_node = create_node(NODE_TYPE_LIST);
                
                int capacity = 4;
                list_node->list.nodes = malloc(sizeof(ASTNode*) * capacity);
                list_node->list.node_count = 0;
                
                while (!parser_match(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
                    if (list_node->list.node_count >= capacity) {
                        capacity *= 2;
                        list_node->list.nodes = realloc(list_node->list.nodes, sizeof(ASTNode*) * capacity);
                    }
                    list_node->list.nodes[list_node->list.node_count++] = parse_expression(parser);
                    
                    if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                        parser_advance(parser);
                    }
                }
                parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
                
                ASTNode* condition = create_condition_node(left, op, list_node);
                return parse_logical_continuation(parser, condition);
            }
        }
        
        // normal comparison (can be arithmetic expression)
        ASTNode* right = parse_expression(parser);
        ASTNode* condition = create_condition_node(left, op, right);
        return parse_logical_continuation(parser, condition);
    }
    
    return left;
}
