#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "evaluator.h"
#include "csv_reader.h"

void test_like_basic() {
    printf("Test: LIKE basic patterns...\n");
    
    // create test CSV
    FILE* f = fopen("test_like_data.csv", "w");
    fprintf(f, "name,role\n");
    fprintf(f, "Alice,admin\n");
    fprintf(f, "Bob,user\n");
    fprintf(f, "Charlie,moderator\n");
    fprintf(f, "Diana,admin\n");
    fprintf(f, "Alex,user\n");
    fclose(f);
    
    // test: names starting with 'A'
    const char* query1 = "SELECT name FROM test_like_data.csv WHERE name LIKE 'A%'";
    ASTNode* ast1 = parse(query1);
    assert(ast1 != NULL);
    
    ResultSet* result1 = evaluate_query(ast1);
    assert(result1 != NULL);
    assert(result1->row_count == 2); // Alice, Alex
    printf("  Names starting with 'A': %d rows\n", result1->row_count);
    
    csv_free(result1);
    releaseNode(ast1);
    
    // test: names ending with 'e'
    const char* query2 = "SELECT name FROM test_like_data.csv WHERE name LIKE '%e'";
    ASTNode* ast2 = parse(query2);
    assert(ast2 != NULL);
    
    ResultSet* result2 = evaluate_query(ast2);
    assert(result2 != NULL);
    assert(result2->row_count == 2); // Alice, Charlie
    printf("  Names ending with 'e': %d rows\n", result2->row_count);
    
    csv_free(result2);
    releaseNode(ast2);
    
    // test: names containing 'li'
    const char* query3 = "SELECT name FROM test_like_data.csv WHERE name LIKE '%li%'";
    ASTNode* ast3 = parse(query3);
    assert(ast3 != NULL);
    
    ResultSet* result3 = evaluate_query(ast3);
    assert(result3 != NULL);
    assert(result3->row_count == 2); // Alice, Charlie
    printf("  Names containing 'li': %d rows\n", result3->row_count);
    
    csv_free(result3);
    releaseNode(ast3);
    
    remove("test_like_data.csv");
    printf("  PASSED\n\n");
}

void test_like_underscore() {
    printf("Test: LIKE with _ (single character)...\n");
    
    FILE* f = fopen("test_like_underscore.csv", "w");
    fprintf(f, "code\n");
    fprintf(f, "A1\n");
    fprintf(f, "A2\n");
    fprintf(f, "B1\n");
    fprintf(f, "AA1\n");
    fclose(f);
    
    // test: codes like 'A_' (A followed by any single character)
    const char* query = "SELECT code FROM test_like_underscore.csv WHERE code LIKE 'A_'";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 2); // A1, A2 (not AA1)
    printf("  Codes matching 'A_': %d rows\n", result->row_count);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_like_underscore.csv");
    printf("  PASSED\n\n");
}

void test_like_case_sensitive() {
    printf("Test: LIKE case sensitivity...\n");
    
    FILE* f = fopen("test_like_case.csv", "w");
    fprintf(f, "name\n");
    fprintf(f, "Alice\n");
    fprintf(f, "alice\n");
    fprintf(f, "ALICE\n");
    fprintf(f, "Bob\n");
    fclose(f);
    
    // LIKE is case-sensitive
    const char* query1 = "SELECT name FROM test_like_case.csv WHERE name LIKE 'alice'";
    ASTNode* ast1 = parse(query1);
    assert(ast1 != NULL);
    
    ResultSet* result1 = evaluate_query(ast1);
    assert(result1 != NULL);
    assert(result1->row_count == 1); // only lowercase 'alice'
    printf("  LIKE 'alice': %d row (case-sensitive)\n", result1->row_count);
    
    csv_free(result1);
    releaseNode(ast1);
    
    remove("test_like_case.csv");
    printf("  PASSED\n\n");
}

void test_ilike_case_insensitive() {
    printf("Test: ILIKE case insensitivity...\n");
    
    FILE* f = fopen("test_ilike_case.csv", "w");
    fprintf(f, "name\n");
    fprintf(f, "Alice\n");
    fprintf(f, "alice\n");
    fprintf(f, "ALICE\n");
    fprintf(f, "Bob\n");
    fclose(f);
    
    // ILIKE is case-insensitive
    const char* query = "SELECT name FROM test_ilike_case.csv WHERE name ILIKE 'alice'";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 3); // Alice, alice, ALICE
    printf("  ILIKE 'alice': %d rows (case-insensitive)\n", result->row_count);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_ilike_case.csv");
    printf("  PASSED\n\n");
}

void test_ilike_patterns() {
    printf("Test: ILIKE with patterns...\n");
    
    FILE* f = fopen("test_ilike_patterns.csv", "w");
    fprintf(f, "email\n");
    fprintf(f, "alice@EXAMPLE.com\n");
    fprintf(f, "bob@example.COM\n");
    fprintf(f, "charlie@OTHER.com\n");
    fprintf(f, "diana@EXAMPLE.org\n");
    fclose(f);
    
    // test: emails from example.com (case-insensitive)
    const char* query = "SELECT email FROM test_ilike_patterns.csv WHERE email ILIKE '%@example.com'";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 2); // alice and bob
    printf("  Emails ending with '@example.com' (case-insensitive): %d rows\n", result->row_count);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_ilike_patterns.csv");
    printf("  PASSED\n\n");
}

void test_like_exact_match() {
    printf("Test: LIKE exact match (no wildcards)...\n");
    
    FILE* f = fopen("test_like_exact.csv", "w");
    fprintf(f, "status\n");
    fprintf(f, "active\n");
    fprintf(f, "inactive\n");
    fprintf(f, "active\n");
    fprintf(f, "pending\n");
    fclose(f);
    
    // exact match without wildcards
    const char* query = "SELECT status FROM test_like_exact.csv WHERE status LIKE 'active'";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 2); // two 'active' rows
    printf("  Status = 'active': %d rows\n", result->row_count);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_like_exact.csv");
    printf("  PASSED\n\n");
}

void test_like_complex_patterns() {
    printf("Test: LIKE complex patterns...\n");
    
    FILE* f = fopen("test_like_complex.csv", "w");
    fprintf(f, "product\n");
    fprintf(f, "USB-001\n");
    fprintf(f, "USB-002\n");
    fprintf(f, "HDMI-100\n");
    fprintf(f, "USB-A-003\n");
    fprintf(f, "VGA-200\n");
    fclose(f);
    
    // pattern: USB-### (USB followed by dash and exactly 3 characters)
    const char* query1 = "SELECT product FROM test_like_complex.csv WHERE product LIKE 'USB-___'";
    ASTNode* ast1 = parse(query1);
    assert(ast1 != NULL);
    
    ResultSet* result1 = evaluate_query(ast1);
    assert(result1 != NULL);
    assert(result1->row_count == 2); // USB-001, USB-002 (not USB-A-003)
    printf("  Products matching 'USB-___': %d rows\n", result1->row_count);
    
    csv_free(result1);
    releaseNode(ast1);
    
    // pattern: starts with USB
    const char* query2 = "SELECT product FROM test_like_complex.csv WHERE product LIKE 'USB%'";
    ASTNode* ast2 = parse(query2);
    assert(ast2 != NULL);
    
    ResultSet* result2 = evaluate_query(ast2);
    assert(result2 != NULL);
    assert(result2->row_count == 3); // USB-001, USB-002, USB-A-003
    printf("  Products starting with 'USB': %d rows\n", result2->row_count);
    
    csv_free(result2);
    releaseNode(ast2);
    
    remove("test_like_complex.csv");
    printf("  PASSED\n\n");
}

void test_like_with_and_or() {
    printf("Test: LIKE with AND/OR conditions...\n");
    
    FILE* f = fopen("test_like_and_or.csv", "w");
    fprintf(f, "name,department\n");
    fprintf(f, "Alice,Sales\n");
    fprintf(f, "Alex,Engineering\n");
    fprintf(f, "Bob,Sales\n");
    fprintf(f, "Amanda,Engineering\n");
    fclose(f);
    
    // test: names starting with 'A' AND department is Sales
    const char* query1 = "SELECT name FROM test_like_and_or.csv WHERE name LIKE 'A%' AND department = 'Sales'";
    ASTNode* ast1 = parse(query1);
    assert(ast1 != NULL);
    
    ResultSet* result1 = evaluate_query(ast1);
    assert(result1 != NULL);
    assert(result1->row_count == 1); // Alice
    printf("  Names starting with 'A' AND Sales: %d row\n", result1->row_count);
    
    csv_free(result1);
    releaseNode(ast1);
    
    // test: names starting with 'A' OR department is Sales
    const char* query2 = "SELECT name FROM test_like_and_or.csv WHERE name LIKE 'A%' OR department = 'Sales'";
    ASTNode* ast2 = parse(query2);
    assert(ast2 != NULL);
    
    ResultSet* result2 = evaluate_query(ast2);
    assert(result2 != NULL);
    assert(result2->row_count == 4); // All rows (Alice, Alex, Bob, Amanda)
    printf("  Names starting with 'A' OR Sales: %d rows\n", result2->row_count);
    
    csv_free(result2);
    releaseNode(ast2);
    
    remove("test_like_and_or.csv");
    printf("  PASSED\n\n");
}

int main() {
    printf("=== LIKE and ILIKE Tests ===\n\n");
    
    test_like_basic();
    test_like_underscore();
    test_like_case_sensitive();
    test_ilike_case_insensitive();
    test_ilike_patterns();
    test_like_exact_match();
    test_like_complex_patterns();
    test_like_with_and_or();
    
    printf("=== All LIKE/ILIKE tests passed! ===\n");
    return 0;
}
