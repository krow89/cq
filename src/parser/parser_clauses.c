/* parser_clauses.c - sql clause parsing functions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "parser.h"
#include "tokenizer.h"
#include "parser/parser_clauses.h"
#include "parser/parser_expressions.h"
#include "parser/parser_core.h"
#include "parser/parser_internal.h"
#include "parser/ast_nodes.h"

/* select clause parsing */
ASTNode* parse_select(Parser* parser) {
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "SELECT")) {
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_TYPE_SELECT);
    
    // check for DISTINCT keyword
    node->select.distinct = false;
    if (parser_match(parser, TOKEN_TYPE_KEYWORD, "DISTINCT")) {
        node->select.distinct = true;
        parser_advance(parser);
    }
    
    int capacity = 4;
    node->select.columns = malloc(sizeof(char*) * capacity);
    node->select.column_nodes = malloc(sizeof(ASTNode*) * capacity);
    node->select.column_count = 0;
    
    // parse column list
    while (1) {
        Token* token = parser_current_token(parser);
        
        // resize arrays if needed
        node->select.columns = ensure_capacity(node->select.columns, &capacity, 
                                               node->select.column_count, sizeof(char*));
        node->select.column_nodes = ensure_capacity(node->select.column_nodes, &capacity, 
                                                    node->select.column_count, sizeof(ASTNode*));
        
        // check for scalar subquery: SELECT ...
        if (token->type == TOKEN_TYPE_PUNCTUATION && strcmp(token->value, "(") == 0) {
            Token* next = parser_peek_token(parser, 1);
            if (next->type == TOKEN_TYPE_KEYWORD && strcasecmp(next->value, "SELECT") == 0) {
                // scalar subquery in SELECT list
                parser_advance(parser); // skip '('
                
                ASTNode* subquery_node = create_node(NODE_TYPE_SUBQUERY);
                subquery_node->subquery.query = parse_query_internal(parser);
                parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")");
                
                // check for AS alias
                char alias_buf[256] = "";
                if (parser_match(parser, TOKEN_TYPE_KEYWORD, "AS")) {
                    parser_advance(parser);
                    Token* alias_token = parser_current_token(parser);
                    if (alias_token->type == TOKEN_TYPE_IDENTIFIER) {
                        snprintf(alias_buf, sizeof(alias_buf), " AS %s", alias_token->value);
                        parser_advance(parser);
                    }
                }
                
                // store the subquery node and a placeholder string
                node->select.column_nodes[node->select.column_count] = subquery_node;
                char col_str[512];
                snprintf(col_str, sizeof(col_str), "(subquery)%s", alias_buf);
                node->select.columns[node->select.column_count] = strdup(col_str);
                node->select.column_count++;
                
                if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                    parser_advance(parser);
                } else {
                    break;
                }
                continue;
            }
        }
        
        // check for SELECT *
        if (token->type == TOKEN_TYPE_OPERATOR && strcmp(token->value, "*") == 0) {
            node->select.columns[node->select.column_count] = strdup("*");
            node->select.column_nodes[node->select.column_count] = NULL;
            node->select.column_count++;
            parser_advance(parser);
            
            if (!parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                break;
            }
            parser_advance(parser); // skip comma
            continue;
        }
        
        // parse general expression handling identifiers, functions, arithmetic, etc.
        ASTNode* expr_node = parse_expression(parser);
        if (!expr_node) break;
        
        // generate column name from expression
        char col_str[512];
        generate_column_name(expr_node, col_str, sizeof(col_str));
        
        // check for AS alias
        if (parser_match(parser, TOKEN_TYPE_KEYWORD, "AS")) {
            parser_advance(parser);
            Token* alias_token = parser_current_token(parser);
            if (alias_token->type == TOKEN_TYPE_IDENTIFIER) {
                // append alias in format " AS alias_name" for evaluator compatibility
                char temp[512];
                snprintf(temp, sizeof(temp), "%s AS %s", col_str, alias_token->value);
                strncpy(col_str, temp, sizeof(col_str) - 1);
                col_str[sizeof(col_str) - 1] = '\0';
                parser_advance(parser);
            }
        }
        
        node->select.columns[node->select.column_count] = strdup(col_str);
        node->select.column_nodes[node->select.column_count] = expr_node;
        node->select.column_count++;
        
        if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
            parser_advance(parser);
        } else {
            break;
        }
    }
    
    return node;
}

/* from clause parsing */
ASTNode* parse_from(Parser* parser) {
    if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "FROM")) {
        return NULL;
    }
    parser_advance(parser);
    
    ASTNode* node = create_node(NODE_TYPE_FROM);
    node->from.table = NULL;
    node->from.subquery = NULL;
    node->from.alias = NULL;
    
    Token* token = parser_current_token(parser);
    
    // check for subquery (parenthesized SELECT)
    if (token->type == TOKEN_TYPE_PUNCTUATION && strcmp(token->value, "(") == 0) {
        parser_advance(parser); // skip '('
        
        // parse the subquery as a full query
        ASTNode* subquery_query = parse_query_internal(parser);
        
        if (!subquery_query) {
            releaseNode(node);
            return NULL;
        }
        
        // expect closing parenthesis
        if (!parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
            releaseNode(subquery_query);
            releaseNode(node);
            return NULL;
        }
        
        // create subquery node and attach the query
        ASTNode* subquery_node = create_node(NODE_TYPE_SUBQUERY);
        subquery_node->subquery.query = subquery_query;
        node->from.subquery = subquery_node;
        
        // subqueries in FROM must have an alias
        token = parser_current_token(parser);
        if (parser_match(parser, TOKEN_TYPE_KEYWORD, "AS")) {
            parser_advance(parser);
            token = parser_current_token(parser);
        }
        
        if (token->type == TOKEN_TYPE_IDENTIFIER) {
            node->from.alias = strdup(token->value);
            parser_advance(parser);
        } else {
            // subquery requires an alias
            fprintf(stderr, "Error: Subquery in FROM clause requires an alias\n");
            releaseNode(node);
            return NULL;
        }
        
        return node;
    }
    
    // expect a string literal filename or identifier table name
    node->from.table = parse_table_name(parser);
    if (!node->from.table) {
        releaseNode(node);
        return NULL;
    }
    
    // parse optional alias
    const char* excluded[] = {"WHERE", "GROUP", "ORDER", "LIMIT", "UNION", "INTERSECT", "EXCEPT"};
    node->from.alias = parse_optional_alias(parser, excluded, 7);
    
    return node;
}

/* where clause parsing */
ASTNode* parse_where(Parser* parser) {
    if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "WHERE")) {
        return NULL;
    }
    parser_advance(parser);
    
    return parse_condition(parser);
}

/* group by clause parsing */
ASTNode* parse_group_by(Parser* parser) {
    if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "GROUP")) {
        return NULL;
    }
    parser_advance(parser);
    
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "BY")) {
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_TYPE_GROUP_BY);
    
    // allocate array for column names
    int capacity = 4;
    node->group_by.columns = malloc(sizeof(char*) * capacity);
    node->group_by.column_count = 0;
    
    // parse first column
    node->group_by.columns[node->group_by.column_count++] = parse_qualified_identifier(parser);
    
    // parse additional columns separated by commas
    while (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
        parser_advance(parser);
        
        // expand array if needed
        if (node->group_by.column_count >= capacity) {
            capacity *= 2;
            node->group_by.columns = realloc(node->group_by.columns, sizeof(char*) * capacity);
        }
        
        node->group_by.columns[node->group_by.column_count++] = parse_qualified_identifier(parser);
    }
    
    return node;
}

/* order by clause parsing */
ASTNode* parse_order_by(Parser* parser) {
    if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "ORDER")) {
        return NULL;
    }
    parser_advance(parser);
    
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "BY")) {
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_TYPE_ORDER_BY);
    node->order_by.descending = false;
    
    // try to parse as function call first
    char* func_str = build_function_string(parser);
    if (func_str) {
        node->order_by.column = func_str;
    } else {
        // parse as qualified identifier
        node->order_by.column = parse_qualified_identifier(parser);
    }
    
    // check for ASC/DESC
    Token* token = parser_current_token(parser);
    if (token->type == TOKEN_TYPE_KEYWORD) {
        if (strcasecmp(token->value, "DESC") == 0) {
            node->order_by.descending = true;
            parser_advance(parser);
        } else if (strcasecmp(token->value, "ASC") == 0) {
            node->order_by.descending = false;
            parser_advance(parser);
        }
    }
    
    return node;
}

/* join clause parsing */
ASTNode* parse_join(Parser* parser) {
    // parse JOIN type (LEFT/RIGHT/FULL/INNER with optional OUTER)
    JoinType join_type = parse_join_type(parser);
    
    // expect JOIN keyword
    if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "JOIN")) {
        return NULL;
    }
    parser_advance(parser);
    
    ASTNode* node = create_node(NODE_TYPE_JOIN);
    node->join.join_type = join_type;
    
    // parse table name
    node->join.table = parse_table_name(parser);
    if (!node->join.table) {
        releaseNode(node);
        return NULL;
    }
    
    // parse optional alias
    const char* excluded[] = {"ON", "WHERE", "GROUP", "ORDER", "LIMIT"};
    node->join.alias = parse_optional_alias(parser, excluded, 5);
    
    // parse ON condition
    if (parser_match(parser, TOKEN_TYPE_KEYWORD, "ON")) {
        parser_advance(parser);
        node->join.condition = parse_condition(parser);
    }
    
    return node;
}

/* helper: parse LIMIT and OFFSET clauses */
void parse_limit_offset(Parser* parser, int* limit, int* offset) {
    if (!parser_match(parser, TOKEN_TYPE_KEYWORD, "LIMIT")) {
        return;
    }
    
    parser_advance(parser);
    Token* limit_token = parser_current_token(parser);
    if (limit_token->type != TOKEN_TYPE_LITERAL) {
        return;
    }
    
    *limit = atoi(limit_token->value);
    parser_advance(parser);
    
    // check for OFFSET or comma syntax
    Token* next = parser_current_token(parser);
    if (next->type == TOKEN_TYPE_PUNCTUATION && strcmp(next->value, ",") == 0) {
        // LIMIT offset, count (MySQL style)
        parser_advance(parser);
        Token* count_token = parser_current_token(parser);
        if (count_token->type == TOKEN_TYPE_LITERAL) {
            *offset = *limit;
            *limit = atoi(count_token->value);
            parser_advance(parser);
        }
    } else if (next->type == TOKEN_TYPE_KEYWORD && strcasecmp(next->value, "OFFSET") == 0) {
        // LIMIT count OFFSET offset (standard SQL)
        parser_advance(parser);
        Token* offset_token = parser_current_token(parser);
        if (offset_token->type == TOKEN_TYPE_LITERAL) {
            *offset = atoi(offset_token->value);
            parser_advance(parser);
        }
    }
}
