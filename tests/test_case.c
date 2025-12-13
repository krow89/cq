#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../include/parser.h"
#include "../include/evaluator.h"
#include "../include/csv_reader.h"

/* Test 1: Simple CASE expression with integer values */
void test_case_simple_integer() {
    const char* query = "SELECT name, CASE age WHEN 25 THEN 'young' WHEN 30 THEN 'mid' ELSE 'other' END AS category FROM 'data/test_data.csv' LIMIT 3";
    
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 3);
    
    assert(strcmp(result->rows[0].values[1].string_value, "young") == 0);   // Alice, 25
    assert(strcmp(result->rows[1].values[1].string_value, "mid") == 0);     // Bob, 30
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_case_simple_integer passed\n");
}

/* Test 2: Searched CASE with conditions */
void test_case_searched_conditions() {
    const char* query = "SELECT name, CASE WHEN age < 28 THEN 'young' WHEN age >= 35 THEN 'mature' ELSE 'mid' END AS category FROM 'data/test_data.csv' LIMIT 4";
    
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 4);
    
    assert(strcmp(result->rows[0].values[1].string_value, "young") == 0);   // Alice, 25
    assert(strcmp(result->rows[2].values[1].string_value, "mature") == 0);  // Charlie, 35
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_case_searched_conditions passed\n");
}

/* Test 3: CASE with numeric results */
void test_case_numeric_results() {
    const char* query = "SELECT name, CASE WHEN age < 30 THEN 1 ELSE 2 END AS tier FROM 'data/test_data.csv' LIMIT 3";
    
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 3);
    
    assert(result->rows[0].values[1].int_value == 1);  // Alice, 25
    assert(result->rows[1].values[1].int_value == 2);  // Bob, 30
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_case_numeric_results passed\n");
}

/* Test 4: Nested CASE expressions */
void test_case_nested() {
    const char* query = "SELECT name, CASE WHEN age < 30 THEN CASE WHEN age < 26 THEN 'very young' ELSE 'young' END ELSE 'older' END AS category FROM 'data/test_data.csv' WHERE name IN ('Alice', 'Diana', 'Bob')";
    
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    
    // alice (25) -> very young, Bob (30) -> older, Diana (28) -> young
    bool found_very_young = false, found_young = false, found_older = false;
    for (int i = 0; i < result->row_count; i++) {
        if (strcmp(result->rows[i].values[1].string_value, "very young") == 0) found_very_young = true;
        if (strcmp(result->rows[i].values[1].string_value, "young") == 0) found_young = true;
        if (strcmp(result->rows[i].values[1].string_value, "older") == 0) found_older = true;
    }
    assert(found_very_young && found_young && found_older);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_case_nested passed\n");
}

/* Test 5: CASE in WHERE clause */
void test_case_in_where() {
    const char* query = "SELECT COUNT(*) FROM 'data/test_data.csv' WHERE CASE WHEN age < 30 THEN 1 ELSE 0 END = 1";
    
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 1);
    
    // should count rows where age < 30
    assert(result->rows[0].values[0].int_value >= 2);  // At least Alice and Diana
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_case_in_where passed\n");
}

/* Test 6: CASE without ELSE (returns NULL) */
void test_case_no_else() {
    const char* query = "SELECT name, CASE WHEN age > 100 THEN 'old' END AS category FROM 'data/test_data.csv' LIMIT 1";
    
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 1);
    
    // should be NULL since no one is > 100
    assert(result->rows[0].values[1].type == VALUE_TYPE_NULL);
    
    csv_free(result);
    releaseNode(ast);
    printf("✓ test_case_no_else passed\n");
}

int main() {
    printf("Running CASE expression tests...\n\n");
    
    test_case_simple_integer();
    test_case_searched_conditions();
    test_case_numeric_results();
    test_case_nested();
    test_case_in_where();
    test_case_no_else();
    
    printf("\n✓ All CASE expression tests passed!\n");
    return 0;
}
