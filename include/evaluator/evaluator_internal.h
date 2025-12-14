#ifndef EVALUATOR_INTERNAL_H
#define EVALUATOR_INTERNAL_H

/* internal header with forward declarations for functions used across modules */

#include "evaluator.h"
#include "parser.h"
#include "csv_reader.h"

/* from evaluator.c */
ResultSet* evaluate_query_internal(ASTNode* query_ast, Row* outer_row, CsvTable* outer_table);

/* from evaluator_aggregates.c */
int find_column_index(CsvTable* table, const char* col_name);
int find_column_index_with_fallback(CsvTable* table, const char* col_name);

/* from evaluator_utils.c */
void trim_trailing_spaces(char* str);
char* extract_column_alias(const char* col_spec);
Value evaluate_column_expression(const char* col_spec, QueryContext* ctx, Row* current_row, int* column_indices, int col_index);

#endif /* EVALUATOR_INTERNAL_H */
