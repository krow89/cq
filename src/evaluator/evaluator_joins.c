#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "evaluator.h"
#include "parser.h"
#include "csv_reader.h"
#include "evaluator/evaluator_joins.h"
#include "evaluator/evaluator_core.h"
#include "evaluator/evaluator_conditions.h"
#include "evaluator/evaluator_utils.h"
#include "evaluator/evaluator_internal.h"

/* helper to set values to NULL */
static void set_null_values(Value* values, int start, int count) {
    for (int i = start; i < start + count; i++) {
        values[i].type = VALUE_TYPE_NULL;
    }
}

/* helper to create a joined row with allocated values */
static Row* create_joined_row(CsvTable* result, int column_count) {
    Row* new_row = &result->rows[result->row_count++];
    new_row->column_count = column_count;
    new_row->values = malloc(sizeof(Value) * column_count);
    return new_row;
}

/* helper to copy table columns to result with alias prefix */
static void copy_columns_with_prefix(Column* dest, int dest_offset, CsvTable* table, const char* alias) {
    for (int i = 0; i < table->column_count; i++) {
        char prefixed_name[256];
        snprintf(prefixed_name, sizeof(prefixed_name), "%s.%s", alias, table->columns[i].name);
        dest[dest_offset + i].name = strdup(prefixed_name);
        dest[dest_offset + i].inferred_type = table->columns[i].inferred_type;
    }
}

/* helper to evaluate JOIN ON condition */
static bool evaluate_join_condition(QueryContext* ctx, ASTNode* on_condition,
                                     Row* left_row, Row* right_row) {
    if (!on_condition) return true;
    
    if (on_condition->type == NODE_TYPE_CONDITION &&
        strcmp(on_condition->condition.operator, "=") == 0 &&
        on_condition->condition.left->type == NODE_TYPE_IDENTIFIER &&
        on_condition->condition.right->type == NODE_TYPE_IDENTIFIER) {
        
        Value* left_val = resolve_column(ctx, on_condition->condition.left->identifier,
                                         left_row, 0);
        Value* right_val = resolve_column(ctx, on_condition->condition.right->identifier,
                                          right_row, 1);
        
        if (left_val && right_val) {
            return (value_compare(left_val, right_val) == 0);
        }
    }
    
    return false;
}

/* simple JOIN implementation that creates a temporary joined table */
static CsvTable* perform_join(QueryContext* ctx, CsvTable* left_table, const char* left_alias,
                               CsvTable* right_table, const char* right_alias,
                               ASTNode* on_condition, JoinType join_type) {
    // create result table with combined columns
    CsvTable* result = calloc(1, sizeof(CsvTable));
    result->filename = strdup("joined_result");
    result->has_header = true;
    result->delimiter = ',';
    
    // combine column names with table prefixes
    result->column_count = left_table->column_count + right_table->column_count;
    result->columns = malloc(sizeof(Column) * result->column_count);
    
    copy_columns_with_prefix(result->columns, 0, left_table, left_alias);
    copy_columns_with_prefix(result->columns, left_table->column_count, right_table, right_alias);
    
    // allocate rows
    result->row_capacity = left_table->row_count * right_table->row_count;
    result->rows = malloc(sizeof(Row) * result->row_capacity);
    result->row_count = 0;
    
    // extend querycontext to include both tables temporarily for condition evaluation
    int orig_table_count = ctx->table_count;
    TableRef* orig_tables = ctx->tables;
    
    ctx->table_count = 2;
    ctx->tables = malloc(sizeof(TableRef) * 2);
    ctx->tables[0].alias = strdup(left_alias);
    ctx->tables[0].table = left_table;
    ctx->tables[1].alias = strdup(right_alias);
    ctx->tables[1].table = right_table;
    
    // perform join
    for (int l = 0; l < left_table->row_count; l++) {
        bool found_match = false;
        
        for (int r = 0; r < right_table->row_count; r++) {
            // create temporary combined row for condition evaluation
            // for condition evaluation we need to check if rows match
            
            bool matches = evaluate_join_condition(ctx, on_condition,
                                                    &left_table->rows[l], &right_table->rows[r]);
            
            if (matches || (join_type == JOIN_TYPE_INNER && on_condition == NULL)) {
                found_match = true;
                
                // create combined row
                Row* new_row = create_joined_row(result, result->column_count);
                
                // copy left table values
                for (int i = 0; i < left_table->column_count; i++) {
                    value_deep_copy(&new_row->values[i], &left_table->rows[l].values[i]);
                }
                
                // copy right table values
                for (int i = 0; i < right_table->column_count; i++) {
                    value_deep_copy(&new_row->values[left_table->column_count + i], &right_table->rows[r].values[i]);
                }
                
                if (result->row_count == 1) {
                    // debug
                }
            }
        }
        
        // left/full join if no match found add left row with nulls for right
        if (!found_match && (join_type == JOIN_TYPE_LEFT || join_type == JOIN_TYPE_FULL)) {
            Row* new_row = create_joined_row(result, result->column_count);
            
            // copy left table values
            for (int i = 0; i < left_table->column_count; i++) {
                value_deep_copy(&new_row->values[i], &left_table->rows[l].values[i]);
            }
            
            // null values for right table
            set_null_values(new_row->values, left_table->column_count, right_table->column_count);
        }
    }
    
    // right/full join: add unmatched rows from right table with nulls for left
    if (join_type == JOIN_TYPE_RIGHT || join_type == JOIN_TYPE_FULL) {
        for (int r = 0; r < right_table->row_count; r++) {
            bool found_match = false;
            
            // check if this right row matched any left row
            for (int l = 0; l < left_table->row_count; l++) {
                bool matches = evaluate_join_condition(ctx, on_condition,
                                                        &left_table->rows[l], &right_table->rows[r]);
                
                if (matches) {
                    found_match = true;
                    break;
                }
            }
            
            // if no match add right row with nulls for left
            if (!found_match) {
                Row* new_row = create_joined_row(result, result->column_count);
                
                // null values for left table
                set_null_values(new_row->values, 0, left_table->column_count);
                
                // copy right table values
                for (int i = 0; i < right_table->column_count; i++) {
                    value_deep_copy(&new_row->values[left_table->column_count + i], &right_table->rows[r].values[i]);
                }
            }
        }
    }
    
    // restore original context
    free(ctx->tables[0].alias);
    free(ctx->tables[1].alias);
    free(ctx->tables);
    ctx->tables = orig_tables;
    ctx->table_count = orig_table_count;
    
    return result;
}

/* load table from FROM clause */
CsvTable* load_from_table(ASTNode* from_clause, const char** out_alias, QueryContext* ctx) {
    (void)ctx; // unused parameter kept for future extensions
    
    if (!from_clause || from_clause->type != NODE_TYPE_FROM) {
        fprintf(stderr, "Error: FROM clause is required\n");
        return NULL;
    }
    
    CsvTable* source_table = NULL;
    const char* table_alias = NULL;
    
    if (from_clause->from.subquery) {
        ASTNode* subquery_node = from_clause->from.subquery;
        if (subquery_node->type != NODE_TYPE_SUBQUERY || !subquery_node->subquery.query) {
            fprintf(stderr, "Error: Invalid subquery in FROM clause\n");
            return NULL;
        }
        
        ResultSet* subquery_result = evaluate_query(subquery_node->subquery.query);
        if (!subquery_result) {
            fprintf(stderr, "Error: Subquery evaluation failed\n");
            return NULL;
        }
        
        source_table = result_to_csv_table(subquery_result);
        csv_free(subquery_result);
        
        if (!source_table) {
            fprintf(stderr, "Error: Failed to convert subquery result to table\n");
            return NULL;
        }
        
        table_alias = from_clause->from.alias ? from_clause->from.alias : "subquery";
    } else if (from_clause->from.table) {
        const char* filename = from_clause->from.table;
        source_table = csv_load(filename, global_csv_config);
        
        if (!source_table) {
            fprintf(stderr, "Failed to load table from '%s'\n", filename);
            return NULL;
        }
        
        table_alias = from_clause->from.alias ? from_clause->from.alias : "main";
    } else {
        fprintf(stderr, "Error: FROM clause must specify a table or subquery\n");
        return NULL;
    }
    
    *out_alias = table_alias;
    return source_table;
}

/* process all JOIN clauses */
CsvTable* process_joins(ASTNode* query_ast, QueryContext* ctx, CsvTable* base_table, const char* base_alias) {
    if (query_ast->query.join_count == 0) {
        return base_table;
    }
    
    CsvTable* working_table = base_table;
    const char* working_alias = base_alias;
    bool joined = false;
    
    for (int j = 0; j < query_ast->query.join_count; j++) {
        ASTNode* join_node = query_ast->query.joins[j];
        if (join_node->type != NODE_TYPE_JOIN) continue;
        
        CsvTable* right_table = csv_load(join_node->join.table, global_csv_config);
        if (!right_table) {
            fprintf(stderr, "Failed to load join table from '%s'\n", join_node->join.table);
            continue;
        }
        
        const char* right_alias = join_node->join.alias ? join_node->join.alias : "right";
        
        CsvTable* joined_table = perform_join(ctx, working_table, working_alias,
                                               right_table, right_alias,
                                               join_node->join.condition,
                                               join_node->join.join_type);
        
        if (joined) {
            csv_free(working_table);
        }
        csv_free(right_table);
        
        working_table = joined_table;
        working_alias = "joined";
        joined = true;
    }
    
    return joined ? working_table : base_table;
}
