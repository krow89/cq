#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "evaluator.h"
#include "csv_reader.h"

void test_distinct_basic() {
    printf("Test: SELECT DISTINCT basic...\n");
    
    // create test CSV with duplicates
    FILE* f = fopen("test_distinct_data.csv", "w");
    fprintf(f, "color,size\n");
    fprintf(f, "red,10\n");
    fprintf(f, "blue,20\n");
    fprintf(f, "red,10\n");
    fprintf(f, "green,15\n");
    fprintf(f, "blue,20\n");
    fprintf(f, "red,10\n");
    fclose(f);
    
    // test without DISTINCT - should have 6 rows
    const char* query1 = "SELECT color, size FROM test_distinct_data.csv";
    ASTNode* ast1 = parse(query1);
    assert(ast1 != NULL);
    
    ResultSet* result1 = evaluate_query(ast1);
    assert(result1 != NULL);
    assert(result1->row_count == 6);
    printf("  Without DISTINCT: %d rows\n", result1->row_count);
    
    csv_free(result1);
    releaseNode(ast1);
    
    // test with DISTINCT - should have 3 unique rows
    const char* query2 = "SELECT DISTINCT color, size FROM test_distinct_data.csv";
    ASTNode* ast2 = parse(query2);
    assert(ast2 != NULL);
    assert(ast2->query.select->select.distinct == true);
    
    ResultSet* result2 = evaluate_query(ast2);
    assert(result2 != NULL);
    assert(result2->row_count == 3);
    printf("  With DISTINCT: %d rows\n", result2->row_count);
    
    // verify the unique values
    csv_print_table(result2, 10);
    
    csv_free(result2);
    releaseNode(ast2);
    
    remove("test_distinct_data.csv");
    printf("  PASSED\n\n");
}

void test_distinct_single_column() {
    printf("Test: SELECT DISTINCT single column...\n");
    
    // create test CSV
    FILE* f = fopen("test_distinct_single.csv", "w");
    fprintf(f, "name,age\n");
    fprintf(f, "Alice,30\n");
    fprintf(f, "Bob,25\n");
    fprintf(f, "Alice,30\n");
    fprintf(f, "Charlie,35\n");
    fprintf(f, "Bob,25\n");
    fclose(f);
    
    const char* query = "SELECT DISTINCT name FROM test_distinct_single.csv";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 3); // Alice, Bob, Charlie
    printf("  Result has %d unique names\n", result->row_count);
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_distinct_single.csv");
    printf("  PASSED\n\n");
}

void test_distinct_with_order_by() {
    printf("Test: SELECT DISTINCT with ORDER BY...\n");
    
    FILE* f = fopen("test_distinct_order.csv", "w");
    fprintf(f, "value\n");
    fprintf(f, "3\n");
    fprintf(f, "1\n");
    fprintf(f, "2\n");
    fprintf(f, "3\n");
    fprintf(f, "1\n");
    fprintf(f, "2\n");
    fclose(f);
    
    const char* query = "SELECT DISTINCT value FROM test_distinct_order.csv ORDER BY value";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 3); // 1, 2, 3
    
    // verify order
    assert(result->rows[0].values[0].int_value == 1);
    assert(result->rows[1].values[0].int_value == 2);
    assert(result->rows[2].values[0].int_value == 3);
    
    printf("  Distinct ordered values: ");
    for (int i = 0; i < result->row_count; i++) {
        printf("%lld ", result->rows[i].values[0].int_value);
    }
    printf("\n");
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_distinct_order.csv");
    printf("  PASSED\n\n");
}

void test_distinct_with_limit() {
    printf("Test: SELECT DISTINCT with LIMIT...\n");
    
    FILE* f = fopen("test_distinct_limit.csv", "w");
    fprintf(f, "num\n");
    fprintf(f, "5\n");
    fprintf(f, "3\n");
    fprintf(f, "5\n");
    fprintf(f, "7\n");
    fprintf(f, "3\n");
    fprintf(f, "9\n");
    fclose(f);
    
    const char* query = "SELECT DISTINCT num FROM test_distinct_limit.csv LIMIT 2";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 2); // only first 2 distinct values
    printf("  Result has %d rows (limited from 4 distinct values)\n", result->row_count);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_distinct_limit.csv");
    printf("  PASSED\n\n");
}

int main() {
    printf("=== DISTINCT Functionality Tests ===\n\n");
    
    test_distinct_basic();
    test_distinct_single_column();
    test_distinct_with_order_by();
    test_distinct_with_limit();
    
    printf("=== All DISTINCT tests passed! ===\n");
    return 0;
}
