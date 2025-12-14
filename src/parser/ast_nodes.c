#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "parser/ast_nodes.h"
#include "string_utils.h"

void retainNode(ASTNode* node) {
    if (node) {
        node->refcount++;
    }
}

void releaseNode(ASTNode* node) {
    if (!node) return;
    
    node->refcount--;
    if (node->refcount > 0) return;
    
    switch (node->type) {
        case NODE_TYPE_QUERY:
            releaseNode(node->query.select);
            releaseNode(node->query.from);
            if (node->query.joins) {
                for (int i = 0; i < node->query.join_count; i++) {
                    releaseNode(node->query.joins[i]);
                }
                free(node->query.joins);
            }
            releaseNode(node->query.where);
            releaseNode(node->query.group_by);
            releaseNode(node->query.having);
            releaseNode(node->query.order_by);
            break;
        case NODE_TYPE_SELECT:
            if (node->select.columns) {
                for (int i = 0; i < node->select.column_count; i++) {
                    free(node->select.columns[i]);
                }
                free(node->select.columns);
            }
            if (node->select.column_nodes) {
                for (int i = 0; i < node->select.column_count; i++) {
                    releaseNode(node->select.column_nodes[i]);
                }
                free(node->select.column_nodes);
            }
            break;
        case NODE_TYPE_CONDITION:
            releaseNode(node->condition.left);
            releaseNode(node->condition.right);
            free(node->condition.operator);
            break;
        case NODE_TYPE_FUNCTION:
            if (node->function.args) {
                for (int i = 0; i < node->function.arg_count; i++) {
                    releaseNode(node->function.args[i]);
                }
                free(node->function.args);
            }
            free(node->function.name);
            break;
        case NODE_TYPE_WINDOW_FUNCTION:
            if (node->window_function.args) {
                for (int i = 0; i < node->window_function.arg_count; i++) {
                    releaseNode(node->window_function.args[i]);
                }
                free(node->window_function.args);
            }
            free(node->window_function.name);
            if (node->window_function.partition_by) {
                for (int i = 0; i < node->window_function.partition_count; i++) {
                    free(node->window_function.partition_by[i]);
                }
                free(node->window_function.partition_by);
            }
            free(node->window_function.order_by_column);
            break;
        case NODE_TYPE_LIST:
            if (node->list.nodes) {
                for (int i = 0; i < node->list.node_count; i++) {
                    releaseNode(node->list.nodes[i]);
                }
                free(node->list.nodes);
            }
            break;
        case NODE_TYPE_ORDER_BY:
            free(node->order_by.column);
            break;
        case NODE_TYPE_GROUP_BY:
            if (node->group_by.columns) {
                for (int i = 0; i < node->group_by.column_count; i++) {
                    free(node->group_by.columns[i]);
                }
                free(node->group_by.columns);
            }
            break;
        case NODE_TYPE_FROM:
            free(node->from.table);
            releaseNode(node->from.subquery);
            free(node->from.alias);
            break;
        case NODE_TYPE_JOIN:
            free(node->join.table);
            free(node->join.alias);
            releaseNode(node->join.condition);
            break;
        case NODE_TYPE_SUBQUERY:
            releaseNode(node->subquery.query);
            break;
        case NODE_TYPE_BINARY_OP:
            releaseNode(node->binary_op.left);
            releaseNode(node->binary_op.right);
            free(node->binary_op.operator);
            break;
        case NODE_TYPE_SET_OP:
            releaseNode(node->set_op.left);
            releaseNode(node->set_op.right);
            break;
        case NODE_TYPE_INSERT:
            free(node->insert.table);
            if (node->insert.columns) {
                for (int i = 0; i < node->insert.column_count; i++) {
                    free(node->insert.columns[i]);
                }
                free(node->insert.columns);
            }
            if (node->insert.values) {
                for (int i = 0; i < node->insert.value_count; i++) {
                    releaseNode(node->insert.values[i]);
                }
                free(node->insert.values);
            }
            break;
        case NODE_TYPE_UPDATE:
            free(node->update.table);
            if (node->update.assignments) {
                for (int i = 0; i < node->update.assignment_count; i++) {
                    releaseNode(node->update.assignments[i]);
                }
                free(node->update.assignments);
            }
            releaseNode(node->update.where);
            break;
        case NODE_TYPE_DELETE:
            free(node->delete_stmt.table);
            releaseNode(node->delete_stmt.where);
            break;
        case NODE_TYPE_CREATE_TABLE:
            free(node->create_table.table);
            if (node->create_table.columns) {
                for (int i = 0; i < node->create_table.column_count; i++) {
                    free(node->create_table.columns[i]);
                }
                free(node->create_table.columns);
            }
            releaseNode(node->create_table.query);
            break;
        case NODE_TYPE_ALTER_TABLE:
            free(node->alter_table.table);
            free(node->alter_table.old_column_name);
            free(node->alter_table.new_column_name);
            break;
        case NODE_TYPE_CASE:
            releaseNode(node->case_expr.case_expr);
            if (node->case_expr.when_exprs) {
                for (int i = 0; i < node->case_expr.when_count; i++) {
                    releaseNode(node->case_expr.when_exprs[i]);
                }
                free(node->case_expr.when_exprs);
            }
            if (node->case_expr.then_exprs) {
                for (int i = 0; i < node->case_expr.when_count; i++) {
                    releaseNode(node->case_expr.then_exprs[i]);
                }
                free(node->case_expr.then_exprs);
            }
            releaseNode(node->case_expr.else_expr);
            break;
        case NODE_TYPE_ASSIGNMENT:
            free(node->assignment.column);
            releaseNode(node->assignment.value);
            break;
        case NODE_TYPE_LITERAL:
            free(node->literal);
            break;
        case NODE_TYPE_IDENTIFIER:
            free(node->identifier);
            break;
        case NODE_TYPE_ALIAS:
            free(node->alias);
            break;
        default:
            break;
    }
    free(node);
}

ASTNode* create_node(ASTNodeType type) {
    ASTNode* node = calloc(1, sizeof(ASTNode));
    node->type = type;
    node->refcount = 1;
    return node;
}

ASTNode* create_identifier_node(const char* name) {
    ASTNode* node = create_node(NODE_TYPE_IDENTIFIER);
    node->identifier = strdup(name);
    return node;
}

ASTNode* create_literal_node(const char* value) {
    ASTNode* node = create_node(NODE_TYPE_LITERAL);
    node->literal = strdup(value);
    return node;
}

ASTNode* create_condition_node(ASTNode* left, const char* op, ASTNode* right) {
    ASTNode* node = create_node(NODE_TYPE_CONDITION);
    node->condition.left = left;
    node->condition.operator = strdup(op);
    node->condition.right = right;
    return node;
}

ASTNode* create_binary_op_node(ASTNode* left, const char* op, ASTNode* right) {
    ASTNode* node = create_node(NODE_TYPE_BINARY_OP);
    node->binary_op.left = left;
    node->binary_op.operator = strdup(op);
    node->binary_op.right = right;
    return node;
}

// generate a column name from an AST node for display
void generate_column_name(ASTNode* node, char* buf, size_t buf_size) {
    if (!node || !buf || buf_size == 0) return;
    
    switch (node->type) {
        case NODE_TYPE_IDENTIFIER:
            if (node->identifier) {
                snprintf(buf, buf_size, "%s", node->identifier);
            } else {
                snprintf(buf, buf_size, "*");
            }
            break;
            
        case NODE_TYPE_LITERAL:
            snprintf(buf, buf_size, "%s", node->literal);
            break;
            
        case NODE_TYPE_FUNCTION: {
            // generate function call string FUNC(args...)
            char args_str[256] = "";
            for (int i = 0; i < node->function.arg_count; i++) {
                if (node->function.args[i] == NULL) {
                    // handle NULL argument, it shouldn't happen, but let's be defensive
                    if (i > 0) cq_strlcat(args_str, ", ", sizeof(args_str));
                    cq_strlcat(args_str, "NULL", sizeof(args_str));
                    continue;
                }
                char arg_buf[128];
                generate_column_name(node->function.args[i], arg_buf, sizeof(arg_buf));
                if (i > 0) cq_strlcat(args_str, ", ", sizeof(args_str));
                cq_strlcat(args_str, arg_buf, sizeof(args_str));
            }
            snprintf(buf, buf_size, "%s(%s)", node->function.name, args_str);
            break;
        }
        
        case NODE_TYPE_WINDOW_FUNCTION: {
            // generate window function string: FUNC(args) OVER (...)
            char args_str[256] = "";
            for (int i = 0; i < node->window_function.arg_count; i++) {
                if (node->window_function.args[i] == NULL) {
                    if (i > 0) cq_strlcat(args_str, ", ", sizeof(args_str));
                    cq_strlcat(args_str, "NULL", sizeof(args_str));
                    continue;
                }
                char arg_buf[128];
                generate_column_name(node->window_function.args[i], arg_buf, sizeof(arg_buf));
                if (i > 0) cq_strlcat(args_str, ", ", sizeof(args_str));
                cq_strlcat(args_str, arg_buf, sizeof(args_str));
            }
            snprintf(buf, buf_size, "%s(%s)", node->window_function.name, args_str);
            break;
        }
            
        case NODE_TYPE_BINARY_OP: {
            // generate arithmetic expression: left op right (or op right for unary)
            char left_str[256] = "", right_str[256] = "";
            
            // handle unary operator (left is NULL)
            if (!node->binary_op.left) {
                generate_column_name(node->binary_op.right, right_str, sizeof(right_str));
                bool right_complex = (node->binary_op.right && node->binary_op.right->type == NODE_TYPE_BINARY_OP);
                if (right_complex) {
                    snprintf(buf, buf_size, "%s(%s)", node->binary_op.operator, right_str);
                } else {
                    snprintf(buf, buf_size, "%s%s", node->binary_op.operator, right_str);
                }
                break;
            }
            
            generate_column_name(node->binary_op.left, left_str, sizeof(left_str));
            generate_column_name(node->binary_op.right, right_str, sizeof(right_str));
            
            // add parentheses for complex sub-expressions
            bool left_complex = (node->binary_op.left->type == NODE_TYPE_BINARY_OP);
            bool right_complex = (node->binary_op.right->type == NODE_TYPE_BINARY_OP);
            
            if (left_complex && right_complex) {
                snprintf(buf, buf_size, "(%s) %s (%s)", left_str, node->binary_op.operator, right_str);
            } else if (left_complex) {
                snprintf(buf, buf_size, "(%s) %s %s", left_str, node->binary_op.operator, right_str);
            } else if (right_complex) {
                snprintf(buf, buf_size, "%s %s (%s)", left_str, node->binary_op.operator, right_str);
            } else {
                snprintf(buf, buf_size, "%s %s %s", left_str, node->binary_op.operator, right_str);
            }
            break;
        }
            
        case NODE_TYPE_SUBQUERY:
            snprintf(buf, buf_size, "(subquery)");
            break;
            
        case NODE_TYPE_CASE:
            snprintf(buf, buf_size, "CASE");
            break;
            
        default:
            snprintf(buf, buf_size, "expr");
            break;
    }
}

// helper to print indentation for AST visualization
static void print_indent(int depth) {
    for (int i = 0; i < depth; i++) printf("  ");
}

void printAst(ASTNode* node, int depth) {
    if (!node) return;
    
    print_indent(depth);
    
    switch (node->type) {
        case NODE_TYPE_QUERY:
            printf("QUERY:\n");
            if (node->query.select) {
                print_indent(depth + 1);
                printf("SELECT:\n");
                printAst(node->query.select, depth + 1);
            }
            if (node->query.from) {
                print_indent(depth + 1);
                printf("FROM:\n");
                printAst(node->query.from, depth + 2);
            }
            if (node->query.where) {
                print_indent(depth + 1);
                printf("WHERE:\n");
                printAst(node->query.where, depth + 2);
            }
            if (node->query.group_by) {
                print_indent(depth + 1);
                printf("GROUP BY:\n");
                printAst(node->query.group_by, depth + 2);
            }
            if (node->query.order_by) {
                print_indent(depth + 1);
                printf("ORDER BY:\n");
                printAst(node->query.order_by, depth + 2);
            }
            break;
        case NODE_TYPE_SELECT:
            for (int i = 0; i < node->select.column_count; i++) {
                print_indent(depth);
                printf("- %s\n", node->select.columns[i]);
            }
            break;
        case NODE_TYPE_GROUP_BY:
            printf("%s\n", node->identifier);
            break;
        case NODE_TYPE_ORDER_BY:
            printf("%s %s\n", node->order_by.column, node->order_by.descending ? "DESC" : "ASC");
            break;
        case NODE_TYPE_FROM:
            printf("Table: %s", node->from.table);
            if (node->from.alias) {
                printf(" AS %s", node->from.alias);
            }
            printf("\n");
            break;
        case NODE_TYPE_IDENTIFIER:
            printf("IDENTIFIER: %s\n", node->identifier);
            break;
        case NODE_TYPE_LITERAL:
            printf("LITERAL: %s\n", node->literal);
            break;
        case NODE_TYPE_CONDITION:
            printf("CONDITION: %s\n", node->condition.operator);
            printAst(node->condition.left, depth + 1);
            printAst(node->condition.right, depth + 1);
            break;
        case NODE_TYPE_FUNCTION:
            printf("FUNCTION: %s\n", node->function.name);
            for (int i = 0; i < node->function.arg_count; i++) {
                printAst(node->function.args[i], depth + 1);
            }
            break;
        case NODE_TYPE_WINDOW_FUNCTION:
            printf("WINDOW FUNCTION: %s\n", node->window_function.name);
            for (int i = 0; i < node->window_function.arg_count; i++) {
                printAst(node->window_function.args[i], depth + 1);
            }
            if (node->window_function.partition_count > 0) {
                print_indent(depth + 1);
                printf("PARTITION BY: ");
                for (int i = 0; i < node->window_function.partition_count; i++) {
                    printf("%s%s", i > 0 ? ", " : "", node->window_function.partition_by[i]);
                }
                printf("\n");
            }
            if (node->window_function.order_by_column) {
                print_indent(depth + 1);
                printf("ORDER BY: %s %s\n", node->window_function.order_by_column,
                       node->window_function.order_descending ? "DESC" : "ASC");
            }
            break;
        case NODE_TYPE_LIST:
            printf("LIST:\n");
            for (int i = 0; i < node->list.node_count; i++) {
                printAst(node->list.nodes[i], depth + 1);
            }
            break;
        case NODE_TYPE_INSERT:
            printf("INSERT INTO: %s\n", node->insert.table);
            if (node->insert.columns) {
                print_indent(depth + 1);
                printf("COLUMNS: ");
                for (int i = 0; i < node->insert.column_count; i++) {
                    printf("%s%s", i > 0 ? ", " : "", node->insert.columns[i]);
                }
                printf("\n");
            }
            print_indent(depth + 1);
            printf("VALUES:\n");
            for (int i = 0; i < node->insert.value_count; i++) {
                printAst(node->insert.values[i], depth + 2);
            }
            break;
        case NODE_TYPE_UPDATE:
            printf("UPDATE: %s\n", node->update.table);
            print_indent(depth + 1);
            printf("SET:\n");
            for (int i = 0; i < node->update.assignment_count; i++) {
                printAst(node->update.assignments[i], depth + 2);
            }
            if (node->update.where) {
                print_indent(depth + 1);
                printf("WHERE:\n");
                printAst(node->update.where, depth + 2);
            }
            break;
        case NODE_TYPE_DELETE:
            printf("DELETE FROM: %s\n", node->delete_stmt.table);
            if (node->delete_stmt.where) {
                print_indent(depth + 1);
                printf("WHERE:\n");
                printAst(node->delete_stmt.where, depth + 2);
            }
            break;
        case NODE_TYPE_CREATE_TABLE:
            printf("CREATE TABLE: %s\n", node->create_table.table);
            if (node->create_table.is_schema_only) {
                print_indent(depth + 1);
                printf("COLUMNS:\n");
                for (int i = 0; i < node->create_table.column_count; i++) {
                    print_indent(depth + 2);
                    printf("%s\n", node->create_table.columns[i]);
                }
            }
            if (node->create_table.query) {
                print_indent(depth + 1);
                printf("AS:\n");
                printAst(node->create_table.query, depth + 2);
            }
            break;
        case NODE_TYPE_ALTER_TABLE:
            printf("ALTER TABLE: %s\n", node->alter_table.table);
            print_indent(depth + 1);
            switch (node->alter_table.operation) {
                case ALTER_RENAME_COLUMN:
                    printf("RENAME COLUMN: %s TO %s\n", 
                           node->alter_table.old_column_name, 
                           node->alter_table.new_column_name);
                    break;
                case ALTER_ADD_COLUMN:
                    printf("ADD COLUMN: %s\n", node->alter_table.new_column_name);
                    break;
                case ALTER_DROP_COLUMN:
                    printf("DROP COLUMN: %s\n", node->alter_table.old_column_name);
                    break;
            }
            break;
        case NODE_TYPE_ASSIGNMENT:
            printf("ASSIGN: %s = ", node->assignment.column);
            if (node->assignment.value) {
                printAst(node->assignment.value, depth);
            }
            break;
        default:
            printf("UNKNOWN NODE (type=%d)\n", node->type);
            break;
    }
}
