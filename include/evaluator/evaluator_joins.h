#ifndef EVALUATOR_JOINS_H
#define EVALUATOR_JOINS_H

#include "evaluator.h"
#include "csv_reader.h"
#include "parser.h"

/* JOIN operations */
CsvTable* load_from_table(ASTNode* from_clause, const char** out_alias, QueryContext* ctx);
CsvTable* process_joins(ASTNode* query_ast, QueryContext* ctx, CsvTable* base_table, const char* base_alias);

#endif /* EVALUATOR_JOINS_H */
