#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include "evaluator.h"
#include "parser.h"
#include "csv_reader.h"
#include "evaluator/evaluator_window.h"
#include "evaluator/evaluator_core.h"
#include "evaluator/evaluator_expressions.h"
#include "evaluator/evaluator_utils.h"
#include "evaluator/evaluator_aggregates.h"
#include "evaluator/evaluator_internal.h"

/* sorting structures and functions */
typedef struct {
    CsvTable* table;
    int column_index;
    bool descending;
} SortContext;

/* global context for generic sorting */
static SortContext* g_sort_ctx = NULL;

/* compare function for row sorting */
static int compare_rows(const void* a, const void* b) {
    Row* row_a = *(Row**)a;
    Row* row_b = *(Row**)b;
    SortContext* sort_ctx = g_sort_ctx;
    
    if (!sort_ctx) return 0;
    
    int col_idx = sort_ctx->column_index;
    if (col_idx < 0 || col_idx >= row_a->column_count) return 0;
    
    Value* val_a = &row_a->values[col_idx];
    Value* val_b = &row_b->values[col_idx];
    
    int cmp = value_compare(val_a, val_b);
    
    return sort_ctx->descending ? -cmp : cmp;
}

/* evaluate window function for all rows */
Value* evaluate_window_function(ASTNode* win_func, QueryContext* ctx, Row** rows, int row_count) {
    if (!win_func || win_func->type != NODE_TYPE_WINDOW_FUNCTION) {
        return NULL;
    }
    
    Value* results = calloc(row_count, sizeof(Value));
    const char* func_name = win_func->window_function.name;
    
    // handle partitioning
    int partition_count = 0;
    int* partition_sizes = NULL;
    int** partition_row_indices = NULL;
    
    if (win_func->window_function.partition_count > 0) {
        // create partitions based on PARTITION BY columns,
        // for now, use a simple implementation with hash map,
        // simplified: just group rows with same partition key
        typedef struct {
            char* key;
            int* row_indices;
            int count;
            int capacity;
        } Partition;
        
        Partition* partitions = NULL;
        int part_capacity = 16;
        partitions = malloc(sizeof(Partition) * part_capacity);
        partition_count = 0;
        
        for (int i = 0; i < row_count; i++) {
            // build partition key from PARTITION BY columns
            char part_key[1024] = "";
            for (int p = 0; p < win_func->window_function.partition_count; p++) {
                Value* val = resolve_column(ctx, win_func->window_function.partition_by[p], rows[i], 0);
                if (val) {
                    if (p > 0) strcat(part_key, "\t");
                    if (val->type == VALUE_TYPE_STRING && val->string_value) {
                        strcat(part_key, val->string_value);
                    } else if (val->type == VALUE_TYPE_INTEGER) {
                        char num_buf[32];
                        snprintf(num_buf, sizeof(num_buf), "%lld", val->int_value);
                        strcat(part_key, num_buf);
                    } else if (val->type == VALUE_TYPE_DOUBLE) {
                        char num_buf[32];
                        snprintf(num_buf, sizeof(num_buf), "%.10g", val->double_value);
                        strcat(part_key, num_buf);
                    }
                }
            }
            
            // find or create partition
            int part_idx = -1;
            for (int p = 0; p < partition_count; p++) {
                if (strcmp(partitions[p].key, part_key) == 0) {
                    part_idx = p;
                    break;
                }
            }
            
            if (part_idx == -1) {
                // create new partition
                if (partition_count >= part_capacity) {
                    part_capacity *= 2;
                    partitions = realloc(partitions, sizeof(Partition) * part_capacity);
                }
                part_idx = partition_count++;
                partitions[part_idx].key = strdup(part_key);
                partitions[part_idx].capacity = 16;
                partitions[part_idx].row_indices = malloc(sizeof(int) * partitions[part_idx].capacity);
                partitions[part_idx].count = 0;
            }
            
            // add row to partition
            if (partitions[part_idx].count >= partitions[part_idx].capacity) {
                partitions[part_idx].capacity *= 2;
                partitions[part_idx].row_indices = realloc(partitions[part_idx].row_indices, 
                    sizeof(int) * partitions[part_idx].capacity);
            }
            partitions[part_idx].row_indices[partitions[part_idx].count++] = i;
        }
        
        // convert partitions to arrays
        partition_sizes = malloc(sizeof(int) * partition_count);
        partition_row_indices = malloc(sizeof(int*) * partition_count);
        for (int p = 0; p < partition_count; p++) {
            partition_sizes[p] = partitions[p].count;
            partition_row_indices[p] = partitions[p].row_indices;
            free(partitions[p].key);
        }
        free(partitions);
    } else {
        // no partitioning, all rows in one partition
        partition_count = 1;
        partition_sizes = malloc(sizeof(int));
        partition_sizes[0] = row_count;
        partition_row_indices = malloc(sizeof(int*));
        partition_row_indices[0] = malloc(sizeof(int) * row_count);
        for (int i = 0; i < row_count; i++) {
            partition_row_indices[0][i] = i;
        }
    }
    
    // sort rows within each partition according to ORDER BY
    if (win_func->window_function.order_by_column) {
        // find the column in the table
        int order_col_idx = -1;
        if (ctx->tables && ctx->table_count > 0) {
            order_col_idx = find_column_index_with_fallback(ctx->tables[0].table, 
                win_func->window_function.order_by_column);
        }
        
        if (order_col_idx >= 0) {
            // sort each partition
            for (int p = 0; p < partition_count; p++) {
                int* indices = partition_row_indices[p];
                int count = partition_sizes[p];
                
                // create temporary array of rows for sorting
                Row** partition_rows = malloc(sizeof(Row*) * count);
                for (int i = 0; i < count; i++) {
                    partition_rows[i] = rows[indices[i]];
                }
                
                // sort the partition
                SortContext sort_ctx;
                sort_ctx.table = ctx->tables[0].table;
                sort_ctx.column_index = order_col_idx;
                sort_ctx.descending = win_func->window_function.order_descending;
                
                g_sort_ctx = &sort_ctx;
                qsort(partition_rows, count, sizeof(Row*), compare_rows);
                g_sort_ctx = NULL;
                
                // update indices to reflect sorted order
                for (int i = 0; i < count; i++) {
                    // find which original index this row corresponds to
                    for (int j = 0; j < row_count; j++) {
                        if (rows[j] == partition_rows[i]) {
                            indices[i] = j;
                            break;
                        }
                    }
                }
                
                free(partition_rows);
            }
        }
    }
    
    // process each partition
    for (int p = 0; p < partition_count; p++) {
        int* indices = partition_row_indices[p];
        int count = partition_sizes[p];
        
        // handle ROW_NUMBER
        if (strcasecmp(func_name, "ROW_NUMBER") == 0) {
            for (int i = 0; i < count; i++) {
                int row_idx = indices[i];
                results[row_idx].type = VALUE_TYPE_INTEGER;
                results[row_idx].int_value = i + 1;
            }
        }
        // handle RANK
        else if (strcasecmp(func_name, "RANK") == 0) {
            if (!win_func->window_function.order_by_column) {
                // RANK requires ORDER BY
                for (int i = 0; i < count; i++) {
                    results[indices[i]].type = VALUE_TYPE_NULL;
                }
                continue;
            }
            
            // assign ranks (with gaps for ties)
            int rank = 1;
            for (int i = 0; i < count; i++) {
                int row_idx = indices[i];
                results[row_idx].type = VALUE_TYPE_INTEGER;
                results[row_idx].int_value = rank;
                
                // check if next row has same value (tie)
                if (i + 1 < count) {
                    Value* curr_val = resolve_column(ctx, win_func->window_function.order_by_column, rows[row_idx], 0);
                    Value* next_val = resolve_column(ctx, win_func->window_function.order_by_column, rows[indices[i + 1]], 0);
                    
                    // if values differ, increment rank by number of tied rows
                    if (curr_val && next_val && value_compare(curr_val, next_val) != 0) {
                        rank = i + 2;
                    }
                }
            }
        }
        // handle DENSE_RANK
        else if (strcasecmp(func_name, "DENSE_RANK") == 0) {
            if (!win_func->window_function.order_by_column) {
                for (int i = 0; i < count; i++) {
                    results[indices[i]].type = VALUE_TYPE_NULL;
                }
                continue;
            }
            
            int dense_rank = 1;
            for (int i = 0; i < count; i++) {
                int row_idx = indices[i];
                results[row_idx].type = VALUE_TYPE_INTEGER;
                results[row_idx].int_value = dense_rank;
                
                // check if next row has different value
                if (i + 1 < count) {
                    Value* curr_val = resolve_column(ctx, win_func->window_function.order_by_column, rows[row_idx], 0);
                    Value* next_val = resolve_column(ctx, win_func->window_function.order_by_column, rows[indices[i + 1]], 0);
                    
                    if (curr_val && next_val && value_compare(curr_val, next_val) != 0) {
                        dense_rank++;
                    }
                }
            }
        }
        // handle LAG
        else if (strcasecmp(func_name, "LAG") == 0) {
            int offset = 1; // default offset
            if (win_func->window_function.arg_count > 1 && 
                win_func->window_function.args[1]->type == NODE_TYPE_LITERAL) {
                Value offset_val = parse_value(win_func->window_function.args[1]->literal, 
                    strlen(win_func->window_function.args[1]->literal));
                if (offset_val.type == VALUE_TYPE_INTEGER) {
                    offset = (int)offset_val.int_value;
                }
            }
            
            for (int i = 0; i < count; i++) {
                int row_idx = indices[i];
                if (i - offset >= 0 && win_func->window_function.arg_count > 0) {
                    int prev_row_idx = indices[i - offset];
                    Value val = evaluate_expression(ctx, win_func->window_function.args[0], rows[prev_row_idx], 0);
                    value_deep_copy(&results[row_idx], &val);
                    if (val.type == VALUE_TYPE_STRING && val.string_value) {
                        free((char*)val.string_value);
                    }
                } else {
                    results[row_idx].type = VALUE_TYPE_NULL;
                }
            }
        }
        // handle LEAD
        else if (strcasecmp(func_name, "LEAD") == 0) {
            int offset = 1;
            if (win_func->window_function.arg_count > 1 && 
                win_func->window_function.args[1]->type == NODE_TYPE_LITERAL) {
                Value offset_val = parse_value(win_func->window_function.args[1]->literal, 
                    strlen(win_func->window_function.args[1]->literal));
                if (offset_val.type == VALUE_TYPE_INTEGER) {
                    offset = (int)offset_val.int_value;
                }
            }
            
            for (int i = 0; i < count; i++) {
                int row_idx = indices[i];
                if (i + offset < count && win_func->window_function.arg_count > 0) {
                    int next_row_idx = indices[i + offset];
                    Value val = evaluate_expression(ctx, win_func->window_function.args[0], rows[next_row_idx], 0);
                    value_deep_copy(&results[row_idx], &val);
                    if (val.type == VALUE_TYPE_STRING && val.string_value) {
                        free((char*)val.string_value);
                    }
                } else {
                    results[row_idx].type = VALUE_TYPE_NULL;
                }
            }
        }
        // handle aggregate window functions (SUM, AVG, COUNT, etc.)
        else if (strcasecmp(func_name, "SUM") == 0 || strcasecmp(func_name, "AVG") == 0 ||
                 strcasecmp(func_name, "COUNT") == 0 || strcasecmp(func_name, "MIN") == 0 ||
                 strcasecmp(func_name, "MAX") == 0) {
            // running aggregate over partition
            for (int i = 0; i < count; i++) {
                int row_idx = indices[i];
                
                // calculate aggregate from start of partition to current row (cumulative)
                Row** partition_rows = malloc(sizeof(Row*) * (i + 1));
                for (int j = 0; j <= i; j++) {
                    partition_rows[j] = rows[indices[j]];
                }
                
                // get column name from first argument
                char col_name[256] = "";
                if (win_func->window_function.arg_count > 0) {
                    if (win_func->window_function.args[0]->type == NODE_TYPE_IDENTIFIER) {
                        strcpy(col_name, win_func->window_function.args[0]->identifier);
                    } else if (win_func->window_function.args[0]->type == NODE_TYPE_LITERAL) {
                        // handle COUNT(*) or similar
                        strcpy(col_name, win_func->window_function.args[0]->literal);
                    }
                }
                
                results[row_idx] = evaluate_aggregate(func_name, partition_rows, i + 1, 
                    ctx->tables[0].table, col_name);
                free(partition_rows);
            }
        }
        else {
            // unknown window function
            for (int i = 0; i < count; i++) {
                results[indices[i]].type = VALUE_TYPE_NULL;
            }
        }
    }
    
    // cleanup
    for (int p = 0; p < partition_count; p++) {
        free(partition_row_indices[p]);
    }
    free(partition_row_indices);
    free(partition_sizes);
    
    return results;
}
