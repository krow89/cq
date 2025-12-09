#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "evaluator.h"
#include "parser.h"
#include "csv_reader.h"

void test_simple_select() {
    printf("Running test_simple_select...\n");
    
    const char* sql = "SELECT name, age FROM 'data/test_data.csv'";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_simple_select passed\n\n");
}

void test_where_filter() {
    printf("Running test_where_filter...\n");
    
    const char* sql = "SELECT name, age FROM 'data/test_data.csv' WHERE age > 30";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    csv_print_table(result, 10);
    
    // should have fewer rows than original
    assert(result->row_count < 7);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_where_filter passed\n\n");
}

void test_where_and() {
    printf("Running test_where_and...\n");
    
    const char* sql = "SELECT name, age, role FROM 'data/test_data.csv' WHERE age > 25 AND active = 1";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_where_and passed\n\n");
}

void test_where_or() {
    printf("Running test_where_or...\n");
    
    const char* sql = "SELECT name, age FROM 'data/test_data.csv' WHERE age < 20 OR age > 40";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_where_or passed\n\n");
}

void test_where_in() {
    printf("Running test_where_in...\n");
    
    const char* sql = "SELECT name, role FROM 'data/test_data.csv' WHERE role IN ('admin', 'moderator')";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_where_in passed\n\n");
}

void test_order_by() {
    printf("Running test_order_by...\n");
    
    const char* sql = "SELECT name, age FROM 'data/test_data.csv' ORDER BY age DESC";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    assert(result->row_count == 7);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    csv_print_table(result, 10);
    
    // check that first row has highest age
    assert(result->rows[0].values[1].int_value == 42);  // Eve, age 42
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_order_by passed\n\n");
}

void test_alias() {
    printf("Running test_alias...\n");
    
    const char* sql = "SELECT name, role AS type, height FROM 'data/test_data.csv'";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    assert(result->column_count == 3);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    
    // check column names
    assert(strcmp(result->columns[0].name, "name") == 0);
    assert(strcmp(result->columns[1].name, "type") == 0);  // should be 'type', not 'role'
    assert(strcmp(result->columns[2].name, "height") == 0);
    
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_alias passed\n\n");
}

void test_group_by_avg() {
    printf("Running test_group_by_avg...\n");
    
    const char* sql = "SELECT role, AVG(height) AS avg_height FROM 'data/test_data.csv' GROUP BY role";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    csv_print_table(result, 10);
    
    // should have 3 groups: admin, user, moderator
    assert(result->row_count == 3);
    assert(result->column_count == 2);
    
    // check column names
    assert(strcmp(result->columns[0].name, "role") == 0);
    assert(strcmp(result->columns[1].name, "avg_height") == 0);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_group_by_avg passed\n\n");
}

void test_group_by_count() {
    printf("Running test_group_by_count...\n");
    
    const char* sql = "SELECT role, COUNT(*) AS count FROM 'data/test_data.csv' GROUP BY role";
    ASTNode* ast = parse(sql);
    
    ResultSet* result = evaluate_query(ast);
    
    assert(result != NULL);
    printf("Result: %d rows, %d columns\n", result->row_count, result->column_count);
    csv_print_table(result, 10);
    
    // should have 3 groups
    assert(result->row_count == 3);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_group_by_count passed\n\n");
}

int main(void) {
    printf("=== Evaluator Test Suite ===\n\n");
    
    test_simple_select();
    test_where_filter();
    test_where_and();
    test_where_or();
    test_where_in();
    test_order_by();
    test_alias();
    test_group_by_avg();
    test_group_by_count();
    
    printf("=== All evaluator tests passed! ===\n");
    return 0;
}
