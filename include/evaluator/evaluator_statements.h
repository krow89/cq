#ifndef EVALUATOR_STATEMENTS_H
#define EVALUATOR_STATEMENTS_H

#include "evaluator.h"
#include "parser.h"

/* DML statement execution */
ResultSet* evaluate_insert(ASTNode* insert_node);
ResultSet* evaluate_update(ASTNode* update_node);
ResultSet* evaluate_delete(ASTNode* delete_node);

/* DDL statement execution */
ResultSet* evaluate_create_table(ASTNode* create_node);
ResultSet* evaluate_alter_table(ASTNode* alter_node);

#endif /* EVALUATOR_STATEMENTS_H */
