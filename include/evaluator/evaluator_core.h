#ifndef EVALUATOR_CORE_H
#define EVALUATOR_CORE_H

#include "evaluator.h"
#include "csv_reader.h"
#include "parser.h"

/* context management */
QueryContext* context_create(ASTNode* query_ast);
void context_free(QueryContext* ctx);

/* table management */
CsvTable* load_table_from_string(const char* filename);
TableRef* context_get_table(QueryContext* ctx, const char* alias);

/* column resolution */
Value* resolve_column(QueryContext* ctx, const char* column_name, Row* current_row, int table_index);

#endif /* EVALUATOR_CORE_H */
