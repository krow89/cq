#ifndef EVALUATOR_UTILS_H
#define EVALUATOR_UTILS_H

#include "evaluator.h"
#include "csv_reader.h"

/* utility functions */
void value_deep_copy(Value* dst, const Value* src);

/* result building */
ResultSet* build_result(QueryContext* ctx, Row** filtered_rows, int row_count);
Row** filter_rows(QueryContext* ctx, ASTNode* where_clause, int* out_filtered_count);

/* result processing */
void sort_result(ResultSet* result, ASTNode* select_node, const char* column_spec, bool descending);
void apply_limit_offset(ResultSet* result, int limit, int offset);
void apply_distinct(ResultSet* result);
void free_row_range(Row* rows, int start, int end);

/* set operations */
ResultSet* set_union(ResultSet* left, ResultSet* right, bool include_duplicates);
ResultSet* set_intersect(ResultSet* left, ResultSet* right);
ResultSet* set_except(ResultSet* left, ResultSet* right);

/* conversion */
CsvTable* result_to_csv_table(ResultSet* result);

#endif /* EVALUATOR_UTILS_H */
