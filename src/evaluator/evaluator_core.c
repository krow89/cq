// evaluator_core.c, core context management functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "evaluator.h"
#include "parser.h"
#include "csv_reader.h"
#include "string_utils.h"
#include "evaluator/evaluator_core.h"
#include "evaluator/evaluator_expressions.h"

// forward declarations
extern CsvConfig global_csv_config;

QueryContext* context_create(ASTNode* query_ast) {
    QueryContext* ctx = calloc(1, sizeof(QueryContext));
    ctx->query = query_ast;
    ctx->tables = NULL;
    ctx->table_count = 0;
    ctx->outer_row = NULL;
    ctx->outer_table = NULL;
    return ctx;
}

void context_free(QueryContext* ctx) {
    if (!ctx) return;
    
    // free loaded tables
    for (int i = 0; i < ctx->table_count; i++) {
        free(ctx->tables[i].alias);
        csv_free(ctx->tables[i].table);
    }
    free(ctx->tables);
    free(ctx);
}

/* table loading */
CsvTable* load_table_from_string(const char* filename) {
    // remove quotes if present
    const char* start = filename;
    const char* end = filename + strlen(filename);
    
    if (*start == '"' || *start == '\'') start++;
    if (end > start && (*(end-1) == '"' || *(end-1) == '\'')) end--;
    
    char* clean_filename = cq_strndup(start, end - start);
    
    CsvConfig config = global_csv_config;
    CsvTable* table = csv_load(clean_filename, config);
    
    free(clean_filename);
    return table;
}

TableRef* context_get_table(QueryContext* ctx, const char* alias) {
    if (!ctx || !alias) return NULL;
    
    for (int i = 0; i < ctx->table_count; i++) {
        if (strcasecmp(ctx->tables[i].alias, alias) == 0) {
            return &ctx->tables[i];
        }
    }
    return NULL;
}

/* function to resolve column by name */
Value* resolve_column(QueryContext* ctx, const char* column_name, Row* current_row, int table_index) {
    if (!ctx || !column_name || !current_row) return NULL;
    
    if (table_index < 0 || table_index >= ctx->table_count) return NULL;
    CsvTable* table = ctx->tables[table_index].table;
    
    // check if it's a qualified name like table.column
    const char* dot = strchr(column_name, '.');
    
    if (dot) {
        // if qualified try exact match first for joined tables
        int col_index = csv_get_column_index(table, column_name);
        if (col_index >= 0) {
            return &current_row->values[col_index];
        }
        
        // if not found, try traditional resolution for table alias lookup
        size_t alias_len = dot - column_name;
        char* table_alias = cq_strndup(column_name, alias_len);
        const char* col_name = dot + 1;
        
        // find the table
        TableRef* table_ref = context_get_table(ctx, table_alias);
        free(table_alias);
        
        if (!table_ref) {
            // if not found in current query check if it's referencing outer table in correlated subquery
            if (ctx->outer_row && ctx->outer_table) {
                col_index = csv_get_column_index(ctx->outer_table, col_name);
                if (col_index >= 0) {
                    return &ctx->outer_row->values[col_index];
                }
            }
            return NULL;
        }
        
        // get column index
        col_index = csv_get_column_index(table_ref->table, col_name);
        if (col_index < 0) {
            // if not found in current query check outer context for correlated subquery
            if (ctx->outer_row && ctx->outer_table) {
                col_index = csv_get_column_index(ctx->outer_table, col_name);
                if (col_index >= 0) {
                    return &ctx->outer_row->values[col_index];
                }
            }
            return NULL;
        }
        
        return &current_row->values[col_index];
    } else {
        // if unqualified look in current table
        int col_index = csv_get_column_index(table, column_name);
        if (col_index < 0) {
            // if not found in current query check outer context for correlated subquery
            if (ctx->outer_row && ctx->outer_table) {
                col_index = csv_get_column_index(ctx->outer_table, column_name);
                if (col_index >= 0) {
                    return &ctx->outer_row->values[col_index];
                }
            }
            
            // EXTENSION: if still not found, check if it's a SELECT alias (non-standard SQL)
            // this allows WHERE to reference computed columns from SELECT
            if (ctx->query && ctx->query->query.select) {
                ASTNode* select_node = ctx->query->query.select;
                if (select_node->type == NODE_TYPE_SELECT && select_node->select.column_nodes) {
                    // look for alias in SELECT columns
                    for (int i = 0; i < select_node->select.column_count; i++) {
                        const char* col_str = select_node->select.columns[i];
                        if (!col_str) continue;
                        
                        // check for " AS alias" pattern
                        const char* as_pos = cq_strcasestr(col_str, " AS ");
                        if (as_pos) {
                            const char* alias_start = as_pos + 4;
                            while (*alias_start && isspace(*alias_start)) alias_start++;
                            
                            // check if this alias matches column_name
                            if (strcasecmp(alias_start, column_name) == 0) {
                                // found the alias! evaluate the expression and store in context
                                // we need to store this temporarily for WHERE evaluation
                                static Value computed_value;
                                computed_value = evaluate_expression(ctx, select_node->select.column_nodes[i], 
                                                                    current_row, table_index);
                                return &computed_value;
                            }
                        }
                    }
                }
            }
            
            return NULL;
        }
        
        return &current_row->values[col_index];
    }
}
