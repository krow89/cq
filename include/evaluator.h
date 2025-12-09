#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "parser.h"
#include "csv_reader.h"

/* table reference with alias */
typedef struct {
    char* alias;          // table alias like "f1", "users"
    CsvTable* table;      // loaded CSV table
} TableRef;

/* query execution context */
typedef struct {
    TableRef* tables;     // array of tables (FROM + JOINs)
    int table_count;
    
    ASTNode* query;       // parsed query AST
    
    /* for correlated subqueries */
    Row* outer_row;       // row from outer query (NULL if not in correlated subquery)
    CsvTable* outer_table; // table from outer query (NULL if not in correlated subquery)
} QueryContext;

/* result set, essentially a CSV table built from query results */
typedef CsvTable ResultSet;

/* global CSV configuration */
extern CsvConfig global_csv_config;

/* main evaluation function */
ResultSet* evaluate_query(ASTNode* query_ast);

/* helper functions */
QueryContext* context_create(ASTNode* query_ast);
void context_free(QueryContext* ctx);

/* load table from filename handling quoted strings */
CsvTable* load_table_from_string(const char* filename);

/* table lookup by alias */
TableRef* context_get_table(QueryContext* ctx, const char* alias);

/* value evaluation */
Value evaluate_expression(QueryContext* ctx, ASTNode* expr, Row* current_row, int table_index);
bool evaluate_condition(QueryContext* ctx, ASTNode* condition, Row* current_row, int table_index);

/* column resolution handling qualified names like "table.column" */
Value* resolve_column(QueryContext* ctx, const char* column_name, Row* current_row, int table_index);

/* result building */
ResultSet* build_result(QueryContext* ctx, Row** filtered_rows, int row_count);

#endif
