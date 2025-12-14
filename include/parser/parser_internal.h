#ifndef PARSER_INTERNAL_H
#define PARSER_INTERNAL_H

#include "parser.h"

/* internal helper functions used across parser modules */

/* helpers from parser_core.c */
void* ensure_capacity(void* array, int* capacity, int current_count, size_t element_size);
char* parse_qualified_identifier(Parser* parser);
char* parse_optional_alias(Parser* parser, const char** excluded_keywords, int excluded_count);
char* parse_table_name(Parser* parser);
JoinType parse_join_type(Parser* parser);
char* build_function_string(Parser* parser);
void parse_limit_offset(Parser* parser, int* limit, int* offset);

/* helpers from ast_nodes.c */
void generate_column_name(ASTNode* node, char* buf, size_t buf_size);

/* helpers for creating nodes */
ASTNode* create_node(ASTNodeType type);
ASTNode* create_condition_node(ASTNode* left, const char* op, ASTNode* right);
ASTNode* create_binary_op_node(ASTNode* left, const char* op, ASTNode* right);

/* from parser.c */
ASTNode* parse_query_internal(Parser* parser);

#endif /* PARSER_INTERNAL_H */
