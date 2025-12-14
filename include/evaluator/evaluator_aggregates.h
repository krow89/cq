#ifndef EVALUATOR_AGGREGATES_H
#define EVALUATOR_AGGREGATES_H

#include "evaluator.h"
#include "parser.h"

/* grouping structures */
typedef struct {
    char* group_key;
    Row** rows;
    int row_count;
    int row_capacity;
} GroupedRows;

typedef struct {
    GroupedRows* groups;
    int group_count;
    int group_capacity;
} GroupResult;

/* aggregate function checking */
bool is_aggregate_function(const char* func_name);
bool has_aggregate_functions(ASTNode* select_node);

/* grouping operations */
GroupResult* create_groups(Row** rows, int row_count, CsvTable* table, const char* group_column);
GroupResult* create_groups_by_expression(QueryContext* ctx, Row** rows, int row_count, ASTNode* group_expr);
void free_groups(GroupResult* groups);

/* aggregate evaluation */
Value evaluate_aggregate(const char* func_name, Row** rows, int row_count, CsvTable* table, const char* column_name);
ResultSet* build_aggregated_result(QueryContext* ctx, GroupResult* groups, ASTNode* select_node);

/* HAVING clause support */
void apply_having_filter(ResultSet* result, ASTNode* having, ASTNode* select_node);

#endif /* EVALUATOR_AGGREGATES_H */
