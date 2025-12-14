// parser.c, main parser module that coordinates all submodules

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "parser.h"
#include "tokenizer.h"
#include "string_utils.h"
#include "parser/parser_core.h"
#include "parser/ast_nodes.h"
#include "parser/parser_expressions.h"
#include "parser/parser_clauses.h"
#include "parser/parser_statements.h"
#include "parser/parser_internal.h"

// forward declarations for functions defined in submodules
ASTNode* parse_select(Parser* parser);
ASTNode* parse_from(Parser* parser);
ASTNode* parse_where(Parser* parser);
ASTNode* parse_group_by(Parser* parser);
ASTNode* parse_order_by(Parser* parser);
ASTNode* parse_join(Parser* parser);
ASTNode* parse_insert(Parser* parser);
ASTNode* parse_update(Parser* parser);
ASTNode* parse_delete(Parser* parser);
ASTNode* parse_create_table(Parser* parser);
ASTNode* parse_alter_table(Parser* parser);
ASTNode* parse_condition(Parser* parser);

// parse internal query, handles SELECT, INSERT, UPDATE, DELETE, CREATE, ALTER
ASTNode* parse_query_internal(Parser* parser) {
    // Check for INSERT, UPDATE, DELETE, CREATE, ALTER first
    Token* first = parser_current_token(parser);
    if (first->type == TOKEN_TYPE_KEYWORD) {
        if (strcasecmp(first->value, "INSERT") == 0) {
            return parse_insert(parser);
        } else if (strcasecmp(first->value, "UPDATE") == 0) {
            return parse_update(parser);
        } else if (strcasecmp(first->value, "DELETE") == 0) {
            return parse_delete(parser);
        } else if (strcasecmp(first->value, "CREATE") == 0) {
            return parse_create_table(parser);
        } else if (strcasecmp(first->value, "ALTER") == 0) {
            return parse_alter_table(parser);
        }
    }
    
    // create root query node to hold all clauses (SELECT)
    ASTNode* root = create_node(NODE_TYPE_QUERY);
    root->query.joins = NULL;
    root->query.join_count = 0;
    root->query.limit = -1;   // no limit by default
    root->query.offset = -1;  // no offset by default
    
    // SELECT
    root->query.select = parse_select(parser);
    if (!root->query.select) {
        releaseNode(root);
        return NULL;
    }
    
    // FROM
    root->query.from = parse_from(parser);
    
    // JOINs
    int join_capacity = 4;
    root->query.joins = malloc(sizeof(ASTNode*) * join_capacity);
    
    while (1) {
        Token* token = parser_current_token(parser);
        if (token->type != TOKEN_TYPE_KEYWORD) break;
        
        if (strcasecmp(token->value, "JOIN") == 0 ||
            strcasecmp(token->value, "LEFT") == 0 ||
            strcasecmp(token->value, "RIGHT") == 0 ||
            strcasecmp(token->value, "FULL") == 0 ||
            strcasecmp(token->value, "INNER") == 0) {
            
            root->query.joins = ensure_capacity(root->query.joins, &join_capacity, 
                                               root->query.join_count, sizeof(ASTNode*));
            
            ASTNode* join_node = parse_join(parser);
            if (join_node) {
                root->query.joins[root->query.join_count++] = join_node;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    // WHERE
    root->query.where = parse_where(parser);
    
    // GROUP BY
    root->query.group_by = parse_group_by(parser);
    
    // HAVING (after GROUP BY)
    root->query.having = NULL;
    if (parser_match(parser, TOKEN_TYPE_KEYWORD, "HAVING")) {
        parser_advance(parser);
        root->query.having = parse_condition(parser);
    }
    
    //ORDER BY
    root->query.order_by = parse_order_by(parser);
    
    // LIMIT/OFFSET
    parse_limit_offset(parser, &root->query.limit, &root->query.offset);
    
    return root;
}

// public API function, parses SQL query and returns AST
ASTNode* parse(const char* sql) {
    int token_count = 0;
    Token* tokens = tokenize(sql, &token_count);
    
    if (!tokens) {
        return NULL;
    }
    
    Parser* parser = parser_init(tokens, token_count);
    
    // parse first query
    ASTNode* left = parse_query_internal(parser);
    if (!left) {
        parser_free(parser);
        freeTokens(tokens, token_count);
        return NULL;
    }
    
    // check for set operations (UNION, INTERSECT, EXCEPT)
    while (1) {
        Token* token = parser_current_token(parser);
        if (token->type != TOKEN_TYPE_KEYWORD) break;
        
        SetOpType op_type;
        bool found_op = false;
        
        if (strcasecmp(token->value, "UNION") == 0) {
            parser_advance(parser);
            // check for UNION ALL
            if (parser_match(parser, TOKEN_TYPE_KEYWORD, "ALL")) {
                parser_advance(parser);
                op_type = SET_OP_UNION_ALL;
            } else {
                op_type = SET_OP_UNION;
            }
            found_op = true;
        } else if (strcasecmp(token->value, "INTERSECT") == 0) {
            parser_advance(parser);
            op_type = SET_OP_INTERSECT;
            found_op = true;
        } else if (strcasecmp(token->value, "EXCEPT") == 0) {
            parser_advance(parser);
            op_type = SET_OP_EXCEPT;
            found_op = true;
        }
        
        if (!found_op) break;
        
        // parse right query
        ASTNode* right = parse_query_internal(parser);
        if (!right) {
            releaseNode(left);
            parser_free(parser);
            freeTokens(tokens, token_count);
            return NULL;
        }
        
        // create set operation node
        ASTNode* set_op = create_node(NODE_TYPE_SET_OP);
        set_op->set_op.op_type = op_type;
        set_op->set_op.left = left;
        set_op->set_op.right = right;
        
        left = set_op;  // for chaining multiple operations
    }
    
    parser_free(parser);
    freeTokens(tokens, token_count);
    
    return left;
}
