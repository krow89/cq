/*
 * evaluator_statements.c
 * dml/ddl statement evaluation (insert, update, delete, create table, alter table)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "evaluator.h"
#include "parser.h"
#include "csv_reader.h"
#include "mmap.h"
#include "evaluator/evaluator_statements.h"
#include "evaluator/evaluator_core.h"
#include "evaluator/evaluator_expressions.h"

extern CsvConfig global_csv_config;

/* evaluate INSERT statement */
ResultSet* evaluate_insert(ASTNode* insert_node) {
    // load existing table
    CsvTable* table = load_table_from_string(insert_node->insert.table);
    if (!table) {
        fprintf(stderr, "Error: Could not load table '%s'\n", insert_node->insert.table);
        return NULL;
    }
    
    // validate column count
    int value_count = insert_node->insert.value_count;
    if (insert_node->insert.columns) {
        // column list specified
        if (insert_node->insert.column_count != value_count) {
            fprintf(stderr, "Error: Column count (%d) does not match value count (%d)\n",
                    insert_node->insert.column_count, value_count);
            csv_free(table);
            return NULL;
        }
    } else {
        // no column list, values must match all columns
        if (value_count != table->column_count) {
            fprintf(stderr, "Error: Value count (%d) does not match table column count (%d)\n",
                    value_count, table->column_count);
            csv_free(table);
            return NULL;
        }
    }
    
    // create new row
    Row new_row;
    new_row.column_count = table->column_count;
    new_row.values = calloc(table->column_count, sizeof(Value));
    
    // initialize all values as NULL
    for (int i = 0; i < table->column_count; i++) {
        new_row.values[i].type = VALUE_TYPE_NULL;
    }
    
    // fill in values
    for (int i = 0; i < value_count; i++) {
        int target_col = i;
        
        // if column list specified, find target column index
        if (insert_node->insert.columns) {
            const char* col_name = insert_node->insert.columns[i];
            target_col = csv_get_column_index(table, col_name);
            if (target_col < 0) {
                fprintf(stderr, "Error: Column '%s' not found in table\n", col_name);
                free(new_row.values);
                csv_free(table);
                return NULL;
            }
        }
        
        // evaluate value expression
        ASTNode* val_node = insert_node->insert.values[i];
        
        // for simple literals, convert directly
        if (val_node->type == NODE_TYPE_LITERAL) {
            const char* literal = val_node->literal;
            Value parsed = parse_value(literal, strlen(literal));
            new_row.values[target_col] = parsed;
        } else if (val_node->type == NODE_TYPE_BINARY_OP) {
            // evaluate arithmetic expression
            QueryContext temp_ctx = {0};
            Value result = evaluate_expression(&temp_ctx, val_node, NULL, 0);
            new_row.values[target_col] = result;
        } else {
            fprintf(stderr, "Error: Unsupported value expression in INSERT\n");
            free(new_row.values);
            csv_free(table);
            return NULL;
        }
    }
    
    // add row to table
    if (table->row_count >= table->row_capacity) {
        table->row_capacity = table->row_capacity ? table->row_capacity * 2 : 8;
        table->rows = realloc(table->rows, sizeof(Row) * table->row_capacity);
    }
    table->rows[table->row_count++] = new_row;
    
    // save table back to file
    if (!csv_save(insert_node->insert.table, table)) {
        fprintf(stderr, "Error: Could not save table '%s'\n", insert_node->insert.table);
        csv_free(table);
        return NULL;
    }
    
    // return result message
    ResultSet* result = malloc(sizeof(ResultSet));
    result->filename = strdup("INSERT result");
    result->data = NULL;
    result->file_size = 0;
    result->fd = -1;
    result->column_count = 1;
    result->columns = malloc(sizeof(Column));
    result->columns[0].name = strdup("message");
    result->columns[0].inferred_type = VALUE_TYPE_STRING;
    result->row_count = 1;
    result->row_capacity = 1;
    result->rows = malloc(sizeof(Row));
    result->rows[0].column_count = 1;
    result->rows[0].values = malloc(sizeof(Value));
    result->rows[0].values[0].type = VALUE_TYPE_STRING;
    result->rows[0].values[0].string_value = malloc(100);
    snprintf(result->rows[0].values[0].string_value, 100, "Inserted 1 row");
    result->has_header = true;
    result->delimiter = ',';
    result->quote = '"';
    
    csv_free(table);
    return result;
}

/* evaluate UPDATE statement */
ResultSet* evaluate_update(ASTNode* update_node) {
    // load existing table
    CsvTable* table = load_table_from_string(update_node->update.table);
    if (!table) {
        fprintf(stderr, "Error: Could not load table '%s'\n", update_node->update.table);
        return NULL;
    }
    
    // create context for condition evaluation
    QueryContext ctx;
    ctx.tables = malloc(sizeof(TableRef));
    ctx.tables[0].alias = strdup("__main__");
    ctx.tables[0].table = table;
    ctx.table_count = 1;
    ctx.query = NULL;
    ctx.outer_row = NULL;
    ctx.outer_table = NULL;
    
    int updated_count = 0;
    
    // process each row
    for (int row = 0; row < table->row_count; row++) {
        // check WHERE condition
        bool matches = true;
        if (update_node->update.where) {
            matches = evaluate_condition(&ctx, update_node->update.where, &table->rows[row], 0);
        }
        
        if (matches) {
            // update columns
            for (int i = 0; i < update_node->update.assignment_count; i++) {
                ASTNode* assignment = update_node->update.assignments[i];
                const char* col_name = assignment->assignment.column;
                
                int col_idx = csv_get_column_index(table, col_name);
                if (col_idx < 0) {
                    fprintf(stderr, "Error: Column '%s' not found\n", col_name);
                    context_free(&ctx);
                    csv_free(table);
                    return NULL;
                }
                
                // Note: We don't free old value because it may point to mmap'd data
                // The value will be overwritten and csv_save will write the new value
                
                // evaluate new value
                ASTNode* val_node = assignment->assignment.value;
                if (val_node->type == NODE_TYPE_LITERAL) {
                    const char* literal = val_node->literal;
                    Value parsed = parse_value(literal, strlen(literal));
                    table->rows[row].values[col_idx] = parsed;
                } else {
                    Value result = evaluate_expression(&ctx, val_node, &table->rows[row], 0);
                    table->rows[row].values[col_idx] = result;
                }
            }
            updated_count++;
        }
    }
    
    // save table back to file
    if (!csv_save(update_node->update.table, table)) {
        fprintf(stderr, "Error: Could not save table '%s'\n", update_node->update.table);
        free(ctx.tables[0].alias);
        free(ctx.tables);
        csv_free(table);
        return NULL;
    }
    
    // return result message
    ResultSet* result = malloc(sizeof(ResultSet));
    result->filename = strdup("UPDATE result");
    result->data = NULL;
    result->file_size = 0;
    result->fd = -1;
    result->column_count = 1;
    result->columns = malloc(sizeof(Column));
    result->columns[0].name = strdup("message");
    result->columns[0].inferred_type = VALUE_TYPE_STRING;
    result->row_count = 1;
    result->row_capacity = 1;
    result->rows = malloc(sizeof(Row));
    result->rows[0].column_count = 1;
    result->rows[0].values = malloc(sizeof(Value));
    result->rows[0].values[0].type = VALUE_TYPE_STRING;
    result->rows[0].values[0].string_value = malloc(100);
    snprintf(result->rows[0].values[0].string_value, 100, "Updated %d row(s)", updated_count);
    result->has_header = true;
    result->delimiter = ',';
    result->quote = '"';
    
    // free context manually (don't use context_free to avoid double-free of table)
    free(ctx.tables[0].alias);
    free(ctx.tables);
    csv_free(table);
    return result;
}

/* evaluate DELETE statement */
ResultSet* evaluate_delete(ASTNode* delete_node) {
    // load existing table
    CsvTable* table = load_table_from_string(delete_node->delete_stmt.table);
    if (!table) {
        fprintf(stderr, "Error: Could not load table '%s'\n", delete_node->delete_stmt.table);
        return NULL;
    }
    
    // create context for condition evaluation
    QueryContext ctx;
    ctx.tables = malloc(sizeof(TableRef));
    ctx.tables[0].alias = strdup("__main__");
    ctx.tables[0].table = table;
    ctx.table_count = 1;
    ctx.query = NULL;
    ctx.outer_row = NULL;
    ctx.outer_table = NULL;
    
    // find rows to delete
    Row** rows_to_keep = malloc(sizeof(Row*) * table->row_count);
    int keep_count = 0;
    int deleted_count = 0;
    
    for (int row = 0; row < table->row_count; row++) {
        bool matches = evaluate_condition(&ctx, delete_node->delete_stmt.where, &table->rows[row], 0);
        
        if (!matches) {
            // keep this row
            rows_to_keep[keep_count++] = &table->rows[row];
        } else {
            // delete this row - free string values
            deleted_count++;
            for (int col = 0; col < table->rows[row].column_count; col++) {
                if (table->rows[row].values[col].type == VALUE_TYPE_STRING) {
                    free(table->rows[row].values[col].string_value);
                }
            }
            free(table->rows[row].values);
        }
    }
    
    // rebuild rows array
    Row* new_rows = malloc(sizeof(Row) * keep_count);
    for (int i = 0; i < keep_count; i++) {
        new_rows[i] = *rows_to_keep[i];
    }
    free(rows_to_keep);
    free(table->rows);
    table->rows = new_rows;
    table->row_count = keep_count;
    table->row_capacity = keep_count;
    
    // save table back to file
    if (!csv_save(delete_node->delete_stmt.table, table)) {
        fprintf(stderr, "Error: Could not save table '%s'\n", delete_node->delete_stmt.table);
        free(ctx.tables[0].alias);
        free(ctx.tables);
        csv_free(table);
        return NULL;
    }
    
    // return result message
    ResultSet* result = malloc(sizeof(ResultSet));
    result->filename = strdup("DELETE result");
    result->data = NULL;
    result->file_size = 0;
    result->fd = -1;
    result->column_count = 1;
    result->columns = malloc(sizeof(Column));
    result->columns[0].name = strdup("message");
    result->columns[0].inferred_type = VALUE_TYPE_STRING;
    result->row_count = 1;
    result->row_capacity = 1;
    result->rows = malloc(sizeof(Row));
    result->rows[0].column_count = 1;
    result->rows[0].values = malloc(sizeof(Value));
    result->rows[0].values[0].type = VALUE_TYPE_STRING;
    result->rows[0].values[0].string_value = malloc(100);
    snprintf(result->rows[0].values[0].string_value, 100, "Deleted %d row(s)", deleted_count);
    result->has_header = true;
    result->delimiter = ',';
    result->quote = '"';
    
    // free context manually, don't use context_free to avoid double-free of table
    free(ctx.tables[0].alias);
    free(ctx.tables);
    csv_free(table);
    return result;
}

/* evaluate CREATE TABLE statement */
ResultSet* evaluate_create_table(ASTNode* create_node) {
    const char* filepath = create_node->create_table.table;
    
    if (create_node->create_table.is_schema_only) {
        // CREATE TABLE 'file.csv' (col1, col2, col3) - create empty file with header
        if (create_node->create_table.column_count == 0) {
            fprintf(stderr, "Error: No columns specified for CREATE TABLE\n");
            return NULL;
        }
        
        // create empty CSV table with just header
        CsvTable* table = malloc(sizeof(CsvTable));
        table->filename = strdup(filepath);
        table->data = NULL;
        table->file_size = 0;
        table->fd = -1;
        table->column_count = create_node->create_table.column_count;
        table->columns = malloc(sizeof(Column) * table->column_count);
        
        for (int i = 0; i < table->column_count; i++) {
            table->columns[i].name = strdup(create_node->create_table.columns[i]);
            table->columns[i].inferred_type = VALUE_TYPE_STRING;  // default type
        }
        
        table->row_count = 0;
        table->row_capacity = 0;
        table->rows = NULL;
        table->has_header = true;
        table->delimiter = ',';
        table->quote = '"';
        
        // save to file
        if (!csv_save(filepath, table)) {
            fprintf(stderr, "Error: Could not create table '%s'\n", filepath);
            csv_free(table);
            return NULL;
        }
        
        csv_free(table);
        
        // return success message
        ResultSet* result = malloc(sizeof(ResultSet));
        result->filename = strdup("CREATE TABLE result");
        result->data = NULL;
        result->file_size = 0;
        result->fd = -1;
        result->column_count = 1;
        result->columns = malloc(sizeof(Column));
        result->columns[0].name = strdup("message");
        result->columns[0].inferred_type = VALUE_TYPE_STRING;
        result->row_count = 1;
        result->row_capacity = 1;
        result->rows = malloc(sizeof(Row));
        result->rows[0].column_count = 1;
        result->rows[0].values = malloc(sizeof(Value));
        result->rows[0].values[0].type = VALUE_TYPE_STRING;
        result->rows[0].values[0].string_value = malloc(100);
        snprintf(result->rows[0].values[0].string_value, 100, 
                "Created table '%s' with %d column(s)", 
                filepath, create_node->create_table.column_count);
        result->has_header = true;
        result->delimiter = ',';
        result->quote = '"';
        
        return result;
        
    } else if (create_node->create_table.query) {
        // CREATE TABLE 'file.csv' AS SELECT ... - save query results
        ResultSet* query_result = evaluate_query(create_node->create_table.query);
        if (!query_result) {
            fprintf(stderr, "Error: Failed to execute query in CREATE TABLE AS\n");
            return NULL;
        }
        
        // save result to file
        if (!csv_save(filepath, query_result)) {
            fprintf(stderr, "Error: Could not save table '%s'\n", filepath);
            csv_free(query_result);
            return NULL;
        }
        
        int saved_rows = query_result->row_count;
        csv_free(query_result);
        
        // return success message
        ResultSet* result = malloc(sizeof(ResultSet));
        result->filename = strdup("CREATE TABLE result");
        result->data = NULL;
        result->file_size = 0;
        result->fd = -1;
        result->column_count = 1;
        result->columns = malloc(sizeof(Column));
        result->columns[0].name = strdup("message");
        result->columns[0].inferred_type = VALUE_TYPE_STRING;
        result->row_count = 1;
        result->row_capacity = 1;
        result->rows = malloc(sizeof(Row));
        result->rows[0].column_count = 1;
        result->rows[0].values = malloc(sizeof(Value));
        result->rows[0].values[0].type = VALUE_TYPE_STRING;
        result->rows[0].values[0].string_value = malloc(100);
        snprintf(result->rows[0].values[0].string_value, 100, 
                "Created table '%s' with %d row(s)", 
                filepath, saved_rows);
        result->has_header = true;
        result->delimiter = ',';
        result->quote = '"';
        
        return result;
    } else {
        fprintf(stderr, "Error: Invalid CREATE TABLE statement\n");
        return NULL;
    }
}

/*
 * evaluate ALTER TABLE statement
 * supports:
 *   - RENAME COLUMN: changes column name in header
 *   - ADD COLUMN: adds new column to end of header (fills with empty values)
 *   - DROP COLUMN: removes column from header and all data
 */
ResultSet* evaluate_alter_table(ASTNode* alter_node) {
    const char* filepath = alter_node->alter_table.table;
    
    // load the CSV file
    CsvConfig config = global_csv_config;
    CsvTable* table = csv_load(filepath, config);
    if (!table) {
        fprintf(stderr, "Error: Could not load table '%s'\n", filepath);
        return NULL;
    }
    
    char message[200];
    bool modified = false;
    
    switch (alter_node->alter_table.operation) {
        case ALTER_RENAME_COLUMN: {
            // find old column
            int col_idx = -1;
            for (int i = 0; i < table->column_count; i++) {
                if (strcasecmp(table->columns[i].name, alter_node->alter_table.old_column_name) == 0) {
                    col_idx = i;
                    break;
                }
            }
            
            if (col_idx == -1) {
                fprintf(stderr, "Error: Column '%s' not found in table\n", 
                       alter_node->alter_table.old_column_name);
                csv_free(table);
                return NULL;
            }
            
            // rename column
            free(table->columns[col_idx].name);
            table->columns[col_idx].name = strdup(alter_node->alter_table.new_column_name);
            
            snprintf(message, sizeof(message), 
                    "Renamed column '%s' to '%s' in table '%s'",
                    alter_node->alter_table.old_column_name,
                    alter_node->alter_table.new_column_name,
                    filepath);
            modified = true;
            break;
        }
        
        case ALTER_ADD_COLUMN: {
            // check if column already exists
            for (int i = 0; i < table->column_count; i++) {
                if (strcasecmp(table->columns[i].name, alter_node->alter_table.new_column_name) == 0) {
                    fprintf(stderr, "Error: Column '%s' already exists in table\n", 
                           alter_node->alter_table.new_column_name);
                    csv_free(table);
                    return NULL;
                }
            }
            
            // add new column to header
            table->columns = realloc(table->columns, sizeof(Column) * (table->column_count + 1));
            table->columns[table->column_count].name = strdup(alter_node->alter_table.new_column_name);
            table->columns[table->column_count].inferred_type = VALUE_TYPE_STRING;
            table->column_count++;
            
            // add empty value to all existing rows
            for (int i = 0; i < table->row_count; i++) {
                table->rows[i].values = realloc(table->rows[i].values, 
                                               sizeof(Value) * table->column_count);
                table->rows[i].values[table->column_count - 1].type = VALUE_TYPE_STRING;
                table->rows[i].values[table->column_count - 1].string_value = strdup("");
                table->rows[i].column_count = table->column_count;
            }
            
            snprintf(message, sizeof(message), 
                    "Added column '%s' to table '%s'",
                    alter_node->alter_table.new_column_name,
                    filepath);
            modified = true;
            break;
        }
        
        case ALTER_DROP_COLUMN: {
            // find column to drop
            int col_idx = -1;
            for (int i = 0; i < table->column_count; i++) {
                if (strcasecmp(table->columns[i].name, alter_node->alter_table.old_column_name) == 0) {
                    col_idx = i;
                    break;
                }
            }
            
            if (col_idx == -1) {
                fprintf(stderr, "Error: Column '%s' not found in table\n", 
                       alter_node->alter_table.old_column_name);
                csv_free(table);
                return NULL;
            }
            
            if (table->column_count == 1) {
                fprintf(stderr, "Error: Cannot drop the last column\n");
                csv_free(table);
                return NULL;
            }
            
            // remove column from header
            free(table->columns[col_idx].name);
            for (int i = col_idx; i < table->column_count - 1; i++) {
                table->columns[i] = table->columns[i + 1];
            }
            table->column_count--;
            table->columns = realloc(table->columns, sizeof(Column) * table->column_count);
            
            // remove column from all rows
            for (int i = 0; i < table->row_count; i++) {
                if (table->rows[i].values[col_idx].type == VALUE_TYPE_STRING) {
                    free(table->rows[i].values[col_idx].string_value);
                }
                
                for (int j = col_idx; j < table->rows[i].column_count - 1; j++) {
                    table->rows[i].values[j] = table->rows[i].values[j + 1];
                }
                
                table->rows[i].column_count--;
                table->rows[i].values = realloc(table->rows[i].values, 
                                               sizeof(Value) * table->rows[i].column_count);
            }
            
            snprintf(message, sizeof(message), 
                    "Dropped column '%s' from table '%s'",
                    alter_node->alter_table.old_column_name,
                    filepath);
            modified = true;
            break;
        }
        
        default:
            fprintf(stderr, "Error: Unknown ALTER TABLE operation\n");
            csv_free(table);
            return NULL;
    }
    
    // save modified table back to file
    if (modified) {
        if (!csv_save(filepath, table)) {
            fprintf(stderr, "Error: Could not save modified table '%s'\n", filepath);
            csv_free(table);
            return NULL;
        }
    }
    
    csv_free(table);
    
    // return success message
    ResultSet* result = malloc(sizeof(ResultSet));
    result->filename = strdup("ALTER TABLE result");
    result->data = NULL;
    result->file_size = 0;
    result->fd = -1;
    result->column_count = 1;
    result->columns = malloc(sizeof(Column));
    result->columns[0].name = strdup("message");
    result->columns[0].inferred_type = VALUE_TYPE_STRING;
    result->row_count = 1;
    result->row_capacity = 1;
    result->rows = malloc(sizeof(Row));
    result->rows[0].column_count = 1;
    result->rows[0].values = malloc(sizeof(Value));
    result->rows[0].values[0].type = VALUE_TYPE_STRING;
    result->rows[0].values[0].string_value = strdup(message);
    result->has_header = true;
    result->delimiter = ',';
    result->quote = '"';
    
    return result;
}
