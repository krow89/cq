#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "parser.h"
#include "tokenizer.h"

/* test helper to check if a node exists and has the expected type */
void assert_node_type(ASTNode* node, ASTNodeType expected_type) {
    assert(node != NULL);
    assert(node->type == expected_type);
}

/* test 1: Simple SELECT */
void test_simple_select() {
    printf("Running test_simple_select...\n");
    
    const char* sql = "SELECT name, age";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert_node_type(ast, NODE_TYPE_QUERY);
    assert(ast->query.select != NULL);
    assert_node_type(ast->query.select, NODE_TYPE_SELECT);
    assert(ast->query.select->select.column_count == 2);
    assert(strcmp(ast->query.select->select.columns[0], "name") == 0);
    assert(strcmp(ast->query.select->select.columns[1], "age") == 0);
    
    releaseNode(ast);
    printf("✓ test_simple_select passed\n\n");
}

/* test 2: SELECT with WHERE */
void test_select_with_where() {
    printf("Running test_select_with_where...\n");
    
    const char* sql = "SELECT name WHERE age > 30";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert_node_type(ast, NODE_TYPE_QUERY);
    assert(ast->query.select != NULL);
    assert(ast->query.where != NULL);
    assert_node_type(ast->query.where, NODE_TYPE_CONDITION);
    assert(strcmp(ast->query.where->condition.operator, ">") == 0);
    
    // check left side (identifier)
    assert(ast->query.where->condition.left != NULL);
    assert_node_type(ast->query.where->condition.left, NODE_TYPE_IDENTIFIER);
    assert(strcmp(ast->query.where->condition.left->identifier, "age") == 0);
    
    // check right side (literal)
    assert(ast->query.where->condition.right != NULL);
    assert_node_type(ast->query.where->condition.right, NODE_TYPE_LITERAL);
    assert(strcmp(ast->query.where->condition.right->literal, "30") == 0);
    
    releaseNode(ast);
    printf("✓ test_select_with_where passed\n\n");
}

/* test 3: WHERE with AND */
void test_where_with_and() {
    printf("Running test_where_with_and...\n");
    
    const char* sql = "SELECT name WHERE age > 30 AND active = 1";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.where != NULL);
    assert_node_type(ast->query.where, NODE_TYPE_CONDITION);
    assert(strcmp(ast->query.where->condition.operator, "AND") == 0);
    
    // left side should be "age > 30"
    ASTNode* left = ast->query.where->condition.left;
    assert(left != NULL);
    assert_node_type(left, NODE_TYPE_CONDITION);
    assert(strcmp(left->condition.operator, ">") == 0);
    
    // right side should be "active = 1"
    ASTNode* right = ast->query.where->condition.right;
    assert(right != NULL);
    assert_node_type(right, NODE_TYPE_CONDITION);
    assert(strcmp(right->condition.operator, "=") == 0);
    
    releaseNode(ast);
    printf("✓ test_where_with_and passed\n\n");
}

/* test 4: WHERE with OR and parentheses */
void test_where_with_or_parentheses() {
    printf("Running test_where_with_or_parentheses...\n");
    
    const char* sql = "SELECT name WHERE (age > 30 OR age < 10)";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.where != NULL);
    assert_node_type(ast->query.where, NODE_TYPE_CONDITION);
    assert(strcmp(ast->query.where->condition.operator, "OR") == 0);
    
    releaseNode(ast);
    printf("✓ test_where_with_or_parentheses passed\n\n");
}

/* test 5: WHERE with IN clause */
void test_where_with_in() {
    printf("Running test_where_with_in...\n");
    
    const char* sql = "SELECT name WHERE role IN ('admin', 'user', 'moderator')";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.where != NULL);
    assert_node_type(ast->query.where, NODE_TYPE_CONDITION);
    assert(strcmp(ast->query.where->condition.operator, "IN") == 0);
    
    // left side should be identifier 'role'
    assert(ast->query.where->condition.left != NULL);
    assert_node_type(ast->query.where->condition.left, NODE_TYPE_IDENTIFIER);
    
    // right side should be a list
    assert(ast->query.where->condition.right != NULL);
    assert_node_type(ast->query.where->condition.right, NODE_TYPE_LIST);
    assert(ast->query.where->condition.right->list.node_count == 3);
    
    releaseNode(ast);
    printf("✓ test_where_with_in passed\n\n");
}

/* test 6: SELECT with function call */
void test_select_with_function() {
    printf("Running test_select_with_function...\n");
    
    const char* sql = "SELECT name, ROUND(height)";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.select != NULL);
    assert(ast->query.select->select.column_count == 2);
    
    // second column should contain function name
    assert(strstr(ast->query.select->select.columns[1], "ROUND") != NULL);
    
    releaseNode(ast);
    printf("✓ test_select_with_function passed\n\n");
}

/* test 7: GROUP BY clause */
void test_group_by() {
    printf("Running test_group_by...\n");
    
    const char* sql = "SELECT role WHERE age > 30 GROUP BY role";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.group_by != NULL);
    assert_node_type(ast->query.group_by, NODE_TYPE_GROUP_BY);
    assert(ast->query.group_by->group_by.column_count == 1);
    assert(strcmp(ast->query.group_by->group_by.columns[0], "role") == 0);
    
    releaseNode(ast);
    printf("✓ test_group_by passed\n\n");
}

/* test 8: ORDER BY clause */
void test_order_by() {
    printf("Running test_order_by...\n");
    
    const char* sql = "SELECT name ORDER BY height DESC";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.order_by != NULL);
    assert_node_type(ast->query.order_by, NODE_TYPE_ORDER_BY);
    assert(strcmp(ast->query.order_by->order_by.column, "height") == 0);
    assert(ast->query.order_by->order_by.descending == true);
    
    releaseNode(ast);
    printf("✓ test_order_by passed\n\n");
}

/* test 9: Complete query */
void test_complete_query() {
    printf("Running test_complete_query...\n");
    
    const char* sql = "SELECT role, name WHERE age > 30 GROUP BY role ORDER BY height DESC";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert_node_type(ast, NODE_TYPE_QUERY);
    
    // check all clauses are present
    assert(ast->query.select != NULL);
    assert(ast->query.where != NULL);
    assert(ast->query.group_by != NULL);
    assert(ast->query.order_by != NULL);
    
    printf("Complete query AST:\n");
    printAst(ast, 0);
    
    releaseNode(ast);
    printf("✓ test_complete_query passed\n\n");
}

/* test 10: Complex WHERE with nested conditions */
void test_complex_where() {
    printf("Running test_complex_where...\n");
    
    const char* sql = "SELECT role, name WHERE (age > 30 OR age < 10) AND active = 1 AND role IN ('user', 'operator')";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.where != NULL);
    
    printf("Complex WHERE AST:\n");
    printAst(ast->query.where, 0);
    
    releaseNode(ast);
    printf("✓ test_complex_where passed\n\n");
}

/* test 11: Multiple comparison operators */
void test_comparison_operators() {
    printf("Running test_comparison_operators...\n");
    
    const char* operators[] = {">", "<", "=", ">=", "<=", "!="};
    
    for (int i = 0; i < 6; i++) {
        char sql[100];
        snprintf(sql, sizeof(sql), "SELECT name WHERE age %s 30", operators[i]);
        
        ASTNode* ast = parse(sql);
        assert(ast != NULL);
        assert(ast->query.where != NULL);
        assert(strcmp(ast->query.where->condition.operator, operators[i]) == 0);
        releaseNode(ast);
    }
    
    printf("✓ test_comparison_operators passed\n\n");
}

/* test 12: Empty clauses */
void test_only_select() {
    printf("Running test_only_select...\n");
    
    const char* sql = "SELECT name, age";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.select != NULL);
    assert(ast->query.where == NULL);
    assert(ast->query.group_by == NULL);
    assert(ast->query.order_by == NULL);
    
    releaseNode(ast);
    printf("✓ test_only_select passed\n\n");
}

/* test 13: GROUP BY with multiple columns */
void test_group_by_multiple() {
    printf("Running test_group_by_multiple...\n");
    
    const char* sql = "SELECT role, age WHERE age > 25 GROUP BY role, age";
    ASTNode* ast = parse(sql);
    
    assert(ast != NULL);
    assert(ast->query.group_by != NULL);
    assert_node_type(ast->query.group_by, NODE_TYPE_GROUP_BY);
    assert(ast->query.group_by->group_by.column_count == 2);
    assert(strcmp(ast->query.group_by->group_by.columns[0], "role") == 0);
    assert(strcmp(ast->query.group_by->group_by.columns[1], "age") == 0);
    
    releaseNode(ast);
    printf("✓ test_group_by_multiple passed\n\n");
}

int main(void) {
    printf("=== Parser/AST Test Suite ===\n\n");
    
    test_simple_select();
    test_select_with_where();
    test_where_with_and();
    test_where_with_or_parentheses();
    test_where_with_in();
    test_select_with_function();
    test_group_by();
    test_group_by_multiple();
    test_order_by();
    test_only_select();
    test_comparison_operators();
    test_complete_query();
    test_complex_where();
    
    printf("=== All parser tests passed! ===\n");
    return 0;
}
