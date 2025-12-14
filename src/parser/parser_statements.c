/*
 * parser_statements.c
 * 
 * DML and DDL statement parsing functions.
 * handles INSERT, UPDATE, DELETE, CREATE TABLE, ALTER TABLE statements.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "parser.h"
#include "tokenizer.h"
#include "parser/parser_statements.h"
#include "parser/parser_expressions.h"
#include "parser/parser_core.h"
#include "parser/parser_internal.h"
#include "parser/ast_nodes.h"

/* parse INSERT INTO table [(col1, col2)] VALUES (val1, val2) */
ASTNode* parse_insert(Parser* parser) {
    // INSERT keyword already verified by caller, just advance
    parser_advance(parser);
    
    // INTO (parser_expect already advances past it)
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "INTO")) {
        fprintf(stderr, "Error: Expected INTO after INSERT\n");
        return NULL;
    }
    
    // table name can be quoted path like './data/test.csv'
    Token* table_token = parser_current_token(parser);
    if (table_token->type != TOKEN_TYPE_IDENTIFIER && table_token->type != TOKEN_TYPE_LITERAL) {
        fprintf(stderr, "Error: Expected table name after INTO\n");
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_TYPE_INSERT);
    node->insert.table = strdup(table_token->value);
    node->insert.columns = NULL;
    node->insert.column_count = 0;
    parser_advance(parser);
    
    // optional column list: (col1, col2, col3)
    if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, "(")) {
        parser_advance(parser);
        
        int capacity = 4;
        node->insert.columns = malloc(sizeof(char*) * capacity);
        
        while (1) {
            Token* col = parser_current_token(parser);
            if (col->type != TOKEN_TYPE_IDENTIFIER) {
                fprintf(stderr, "Error: Expected column name in INSERT column list\n");
                releaseNode(node);
                return NULL;
            }
            
            if (node->insert.column_count >= capacity) {
                capacity *= 2;
                node->insert.columns = realloc(node->insert.columns, sizeof(char*) * capacity);
            }
            
            node->insert.columns[node->insert.column_count++] = strdup(col->value);
            parser_advance(parser);
            
            if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                parser_advance(parser);
            } else {
                break;
            }
        }
        
        if (!parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
            fprintf(stderr, "Error: Expected ')' after column list\n");
            releaseNode(node);
            return NULL;
        }
    }
    
    // VALUES (parser_expect already advances)
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "VALUES")) {
        fprintf(stderr, "Error: Expected VALUES in INSERT statement\n");
        releaseNode(node);
        return NULL;
    }
    
    // (value1, value2, value3) (parser_expect already advances)
    if (!parser_expect(parser, TOKEN_TYPE_PUNCTUATION, "(")) {
        fprintf(stderr, "Error: Expected '(' after VALUES\n");
        releaseNode(node);
        return NULL;
    }
    
    int capacity = 4;
    node->insert.values = malloc(sizeof(ASTNode*) * capacity);
    node->insert.value_count = 0;
    
    while (1) {
        ASTNode* value = parse_expression(parser);
        if (!value) {
            fprintf(stderr, "Error: Expected value in VALUES list\n");
            releaseNode(node);
            return NULL;
        }
        
        if (node->insert.value_count >= capacity) {
            capacity *= 2;
            node->insert.values = realloc(node->insert.values, sizeof(ASTNode*) * capacity);
        }
        
        node->insert.values[node->insert.value_count++] = value;
        
        if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
            parser_advance(parser);
        } else {
            break;
        }
    }
    
    if (!parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
        fprintf(stderr, "Error: Expected ')' after VALUES list\n");
        releaseNode(node);
        return NULL;
    }
    
    return node;
}

/* parse UPDATE table SET col1=val1, col2=val2 WHERE condition */
ASTNode* parse_update(Parser* parser) {
    // UPDATE keyword already verified by caller, just advance
    parser_advance(parser);
    
    // table name
    Token* table_token = parser_current_token(parser);
    if (table_token->type != TOKEN_TYPE_IDENTIFIER && table_token->type != TOKEN_TYPE_LITERAL) {
        fprintf(stderr, "Error: Expected table name after UPDATE\n");
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_TYPE_UPDATE);
    node->update.table = strdup(table_token->value);
    node->update.assignments = NULL;
    node->update.assignment_count = 0;
    node->update.where = NULL;
    parser_advance(parser);
    
    // SET (parser_expect already advances)
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "SET")) {
        fprintf(stderr, "Error: Expected SET after table name in UPDATE\n");
        releaseNode(node);
        return NULL;
    }
    
    // parse assignments col1=val1, col2=val2
    int capacity = 4;
    node->update.assignments = malloc(sizeof(ASTNode*) * capacity);
    
    while (1) {
        Token* col = parser_current_token(parser);
        if (col->type != TOKEN_TYPE_IDENTIFIER) {
            fprintf(stderr, "Error: Expected column name in SET clause\n");
            releaseNode(node);
            return NULL;
        }
        
        ASTNode* assignment = create_node(NODE_TYPE_ASSIGNMENT);
        assignment->assignment.column = strdup(col->value);
        parser_advance(parser);
        
        // = (parser_expect already advances)
        if (!parser_expect(parser, TOKEN_TYPE_OPERATOR, "=")) {
            fprintf(stderr, "Error: Expected '=' in assignment\n");
            releaseNode(assignment);
            releaseNode(node);
            return NULL;
        }
        
        // value
        assignment->assignment.value = parse_expression(parser);
        if (!assignment->assignment.value) {
            fprintf(stderr, "Error: Expected value in assignment\n");
            releaseNode(assignment);
            releaseNode(node);
            return NULL;
        }
        
        if (node->update.assignment_count >= capacity) {
            capacity *= 2;
            node->update.assignments = realloc(node->update.assignments, sizeof(ASTNode*) * capacity);
        }
        
        node->update.assignments[node->update.assignment_count++] = assignment;
        
        if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
            parser_advance(parser);
        } else {
            break;
        }
    }
    
    // optional WHERE
    node->update.where = parse_where(parser);
    
    return node;
}

/* parse DELETE FROM table WHERE condition */
ASTNode* parse_delete(Parser* parser) {
    // DELETE keyword already verified by caller, just advance
    parser_advance(parser);
    
    // FROM (parser_expect already advances)
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "FROM")) {
        fprintf(stderr, "Error: Expected FROM after DELETE\n");
        return NULL;
    }
    
    // table name
    Token* table_token = parser_current_token(parser);
    if (table_token->type != TOKEN_TYPE_IDENTIFIER && table_token->type != TOKEN_TYPE_LITERAL) {
        fprintf(stderr, "Error: Expected table name after FROM\n");
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_TYPE_DELETE);
    node->delete_stmt.table = strdup(table_token->value);
    node->delete_stmt.where = NULL;
    parser_advance(parser);
    
    // WHERE, required for safety, we don't allow DELETE without WHERE
    node->delete_stmt.where = parse_where(parser);
    if (!node->delete_stmt.where) {
        fprintf(stderr, "Error: WHERE clause is required for DELETE (safety measure)\n");
        releaseNode(node);
        return NULL;
    }
    
    return node;
}

/* parse CREATE TABLE 'file.csv' AS SELECT ... 
 *    or CREATE TABLE 'file.csv' (col1, col2, col3)
 *    or CREATE TABLE 'file.csv' AS (col1, col2, col3)
 */
ASTNode* parse_create_table(Parser* parser) {
    // CREATE keyword already verified by caller, just advance
    parser_advance(parser);
    
    // TABLE keyword
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "TABLE")) {
        fprintf(stderr, "Error: Expected TABLE after CREATE\n");
        return NULL;
    }
    
    // table name (file path)
    Token* table_token = parser_current_token(parser);
    if (table_token->type != TOKEN_TYPE_IDENTIFIER && table_token->type != TOKEN_TYPE_LITERAL) {
        fprintf(stderr, "Error: Expected table name/path after CREATE TABLE\n");
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_TYPE_CREATE_TABLE);
    node->create_table.table = strdup(table_token->value);
    node->create_table.columns = NULL;
    node->create_table.column_count = 0;
    node->create_table.query = NULL;
    node->create_table.is_schema_only = false;
    parser_advance(parser);
    
    // check next token: AS or (
    if (parser_match(parser, TOKEN_TYPE_KEYWORD, "AS")) {
        parser_advance(parser);
        
        if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, "(")) {
            // could be AS (col1, col2) or AS (SELECT ...)
            // peek to see if next token is SELECT
            Token* peek = parser_peek_token(parser, 1);
            if (peek && peek->type == TOKEN_TYPE_KEYWORD && 
                strcasecmp(peek->value, "SELECT") == 0) {
                // AS (SELECT ...) for a subquery
                parser_advance(parser); // consume (
                node->create_table.query = parse_query_internal(parser);
                if (!node->create_table.query) {
                    fprintf(stderr, "Error: Failed to parse SELECT query in CREATE TABLE AS\n");
                    releaseNode(node);
                    return NULL;
                }
                if (!parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
                    fprintf(stderr, "Error: Expected ')' after SELECT query\n");
                    releaseNode(node);
                    return NULL;
                }
            } else {
                // AS (col1, col2, col3) for schema mapping
                parser_advance(parser); // consume (
                int capacity = 4;
                node->create_table.columns = malloc(sizeof(char*) * capacity);
                
                while (1) {
                    Token* col = parser_current_token(parser);
                    if (col->type != TOKEN_TYPE_IDENTIFIER) {
                        fprintf(stderr, "Error: Expected column name in schema definition\n");
                        releaseNode(node);
                        return NULL;
                    }
                    
                    if (node->create_table.column_count >= capacity) {
                        capacity *= 2;
                        node->create_table.columns = realloc(node->create_table.columns, 
                                                             sizeof(char*) * capacity);
                    }
                    
                    node->create_table.columns[node->create_table.column_count++] = strdup(col->value);
                    parser_advance(parser);
                    
                    if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                        parser_advance(parser);
                    } else {
                        break;
                    }
                }
                
                if (!parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
                    fprintf(stderr, "Error: Expected ')' after column list\n");
                    releaseNode(node);
                    return NULL;
                }
                // this is schema mapping but we'll treat as schema only for now
                node->create_table.is_schema_only = true;
            }
        } else {
            // AS SELECT without parentheses
            node->create_table.query = parse_query_internal(parser);
            if (!node->create_table.query) {
                fprintf(stderr, "Error: Failed to parse SELECT query in CREATE TABLE AS\n");
                releaseNode(node);
                return NULL;
            }
        }
    } else if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, "(")) {
        // CREATE TABLE 'file' (col1, col2, col3) for empty table with schema
        parser_advance(parser);
        int capacity = 4;
        node->create_table.columns = malloc(sizeof(char*) * capacity);
        
        while (1) {
            Token* col = parser_current_token(parser);
            if (col->type != TOKEN_TYPE_IDENTIFIER) {
                fprintf(stderr, "Error: Expected column name in CREATE TABLE\n");
                releaseNode(node);
                return NULL;
            }
            
            if (node->create_table.column_count >= capacity) {
                capacity *= 2;
                node->create_table.columns = realloc(node->create_table.columns, 
                                                     sizeof(char*) * capacity);
            }
            
            node->create_table.columns[node->create_table.column_count++] = strdup(col->value);
            parser_advance(parser);
            
            if (parser_match(parser, TOKEN_TYPE_PUNCTUATION, ",")) {
                parser_advance(parser);
            } else {
                break;
            }
        }
        
        if (!parser_expect(parser, TOKEN_TYPE_PUNCTUATION, ")")) {
            fprintf(stderr, "Error: Expected ')' after column list\n");
            releaseNode(node);
            return NULL;
        }
        node->create_table.is_schema_only = true;
    } else {
        fprintf(stderr, "Error: Expected AS or '(' after table name in CREATE TABLE\n");
        releaseNode(node);
        return NULL;
    }
    
    return node;
}

/*
 * parse ALTER TABLE statement,
 * supports:
 *   ALTER TABLE 'file.csv' RENAME COLUMN old_name TO new_name
 *   ALTER TABLE 'file.csv' ADD COLUMN new_name
 *   ALTER TABLE 'file.csv' DROP COLUMN col_name
 */
ASTNode* parse_alter_table(Parser* parser) {
    // ALTER keyword already verified by caller, just advance
    parser_advance(parser);
    
    // TABLE keyword
    if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "TABLE")) {
        fprintf(stderr, "Error: Expected TABLE after ALTER\n");
        return NULL;
    }
    
    // table name (file path)
    Token* table_token = parser_current_token(parser);
    if (table_token->type != TOKEN_TYPE_IDENTIFIER && table_token->type != TOKEN_TYPE_LITERAL) {
        fprintf(stderr, "Error: Expected table name/path after ALTER TABLE\n");
        return NULL;
    }
    
    ASTNode* node = create_node(NODE_TYPE_ALTER_TABLE);
    node->alter_table.table = strdup(table_token->value);
    node->alter_table.old_column_name = NULL;
    node->alter_table.new_column_name = NULL;
    parser_advance(parser);
    
    // check operation: RENAME, ADD, or DROP
    Token* op_token = parser_current_token(parser);
    if (!op_token || op_token->type != TOKEN_TYPE_KEYWORD) {
        fprintf(stderr, "Error: Expected RENAME, ADD, or DROP after table name\n");
        releaseNode(node);
        return NULL;
    }
    
    if (strcasecmp(op_token->value, "RENAME") == 0) {
        // RENAME COLUMN old_name TO new_name
        node->alter_table.operation = ALTER_RENAME_COLUMN;
        parser_advance(parser);
        
        if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "COLUMN")) {
            fprintf(stderr, "Error: Expected COLUMN after RENAME\n");
            releaseNode(node);
            return NULL;
        }
        
        Token* old_col = parser_current_token(parser);
        if (old_col->type != TOKEN_TYPE_IDENTIFIER) {
            fprintf(stderr, "Error: Expected column name after RENAME COLUMN\n");
            releaseNode(node);
            return NULL;
        }
        node->alter_table.old_column_name = strdup(old_col->value);
        parser_advance(parser);
        
        if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "TO")) {
            fprintf(stderr, "Error: Expected TO after old column name\n");
            releaseNode(node);
            return NULL;
        }
        
        Token* new_col = parser_current_token(parser);
        if (new_col->type != TOKEN_TYPE_IDENTIFIER) {
            fprintf(stderr, "Error: Expected new column name after TO\n");
            releaseNode(node);
            return NULL;
        }
        node->alter_table.new_column_name = strdup(new_col->value);
        parser_advance(parser);
        
    } else if (strcasecmp(op_token->value, "ADD") == 0) {
        // ADD COLUMN new_name
        node->alter_table.operation = ALTER_ADD_COLUMN;
        parser_advance(parser);
        
        if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "COLUMN")) {
            fprintf(stderr, "Error: Expected COLUMN after ADD\n");
            releaseNode(node);
            return NULL;
        }
        
        Token* new_col = parser_current_token(parser);
        if (new_col->type != TOKEN_TYPE_IDENTIFIER) {
            fprintf(stderr, "Error: Expected column name after ADD COLUMN\n");
            releaseNode(node);
            return NULL;
        }
        node->alter_table.new_column_name = strdup(new_col->value);
        parser_advance(parser);
        
    } else if (strcasecmp(op_token->value, "DROP") == 0) {
        // DROP COLUMN col_name
        node->alter_table.operation = ALTER_DROP_COLUMN;
        parser_advance(parser);
        
        if (!parser_expect(parser, TOKEN_TYPE_KEYWORD, "COLUMN")) {
            fprintf(stderr, "Error: Expected COLUMN after DROP\n");
            releaseNode(node);
            return NULL;
        }
        
        Token* col = parser_current_token(parser);
        if (col->type != TOKEN_TYPE_IDENTIFIER) {
            fprintf(stderr, "Error: Expected column name after DROP COLUMN\n");
            releaseNode(node);
            return NULL;
        }
        node->alter_table.old_column_name = strdup(col->value);
        parser_advance(parser);
        
    } else {
        fprintf(stderr, "Error: Unsupported ALTER TABLE operation '%s'\n", op_token->value);
        releaseNode(node);
        return NULL;
    }
    
    return node;
}
