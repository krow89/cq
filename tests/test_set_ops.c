#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "evaluator.h"
#include "csv_reader.h"

void test_union() {
    printf("Test: UNION...\n");
    
    // create two test CSVs
    FILE* f1 = fopen("test_set_a.csv", "w");
    fprintf(f1, "id,name\n");
    fprintf(f1, "1,Alice\n");
    fprintf(f1, "2,Bob\n");
    fprintf(f1, "3,Charlie\n");
    fclose(f1);
    
    FILE* f2 = fopen("test_set_b.csv", "w");
    fprintf(f2, "id,name\n");
    fprintf(f2, "2,Bob\n");
    fprintf(f2, "3,Charlie\n");
    fprintf(f2, "4,Diana\n");
    fclose(f2);
    
    // test UNION (removes duplicates)
    const char* query = "SELECT * FROM test_set_a.csv UNION SELECT * FROM test_set_b.csv";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 4); // Alice, Bob, Charlie, Diana (Bob and Charlie not duplicated)
    printf("  UNION result: %d unique rows\n", result->row_count);
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_set_a.csv");
    remove("test_set_b.csv");
    printf("  PASSED\n\n");
}

void test_union_all() {
    printf("Test: UNION ALL...\n");
    
    FILE* f1 = fopen("test_set_c.csv", "w");
    fprintf(f1, "value\n");
    fprintf(f1, "1\n");
    fprintf(f1, "2\n");
    fclose(f1);
    
    FILE* f2 = fopen("test_set_d.csv", "w");
    fprintf(f2, "value\n");
    fprintf(f2, "2\n");
    fprintf(f2, "3\n");
    fclose(f2);
    
    // test UNION ALL (keeps duplicates)
    const char* query = "SELECT * FROM test_set_c.csv UNION ALL SELECT * FROM test_set_d.csv";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 4); // 1, 2, 2, 3 (keeps duplicate 2)
    printf("  UNION ALL result: %d rows (includes duplicates)\n", result->row_count);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_set_c.csv");
    remove("test_set_d.csv");
    printf("  PASSED\n\n");
}

void test_intersect() {
    printf("Test: INTERSECT...\n");
    
    FILE* f1 = fopen("test_intersect_a.csv", "w");
    fprintf(f1, "id,name\n");
    fprintf(f1, "1,Alice\n");
    fprintf(f1, "2,Bob\n");
    fprintf(f1, "3,Charlie\n");
    fclose(f1);
    
    FILE* f2 = fopen("test_intersect_b.csv", "w");
    fprintf(f2, "id,name\n");
    fprintf(f2, "2,Bob\n");
    fprintf(f2, "3,Charlie\n");
    fprintf(f2, "4,Diana\n");
    fclose(f2);
    
    const char* query = "SELECT * FROM test_intersect_a.csv INTERSECT SELECT * FROM test_intersect_b.csv";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 2); // Bob, Charlie (common to both)
    printf("  INTERSECT result: %d common rows\n", result->row_count);
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_intersect_a.csv");
    remove("test_intersect_b.csv");
    printf("  PASSED\n\n");
}

void test_except() {
    printf("Test: EXCEPT...\n");
    
    FILE* f1 = fopen("test_except_a.csv", "w");
    fprintf(f1, "id,name\n");
    fprintf(f1, "1,Alice\n");
    fprintf(f1, "2,Bob\n");
    fprintf(f1, "3,Charlie\n");
    fclose(f1);
    
    FILE* f2 = fopen("test_except_b.csv", "w");
    fprintf(f2, "id,name\n");
    fprintf(f2, "2,Bob\n");
    fprintf(f2, "3,Charlie\n");
    fprintf(f2, "4,Diana\n");
    fclose(f2);
    
    const char* query = "SELECT * FROM test_except_a.csv EXCEPT SELECT * FROM test_except_b.csv";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 1); // Alice (only in first, not in second)
    printf("  EXCEPT result: %d rows\n", result->row_count);
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_except_a.csv");
    remove("test_except_b.csv");
    printf("  PASSED\n\n");
}

void test_union_with_where() {
    printf("Test: UNION with WHERE clauses...\n");
    
    FILE* f = fopen("test_union_where.csv", "w");
    fprintf(f, "id,value\n");
    fprintf(f, "1,10\n");
    fprintf(f, "2,20\n");
    fprintf(f, "3,30\n");
    fprintf(f, "4,40\n");
    fclose(f);
    
    const char* query = "SELECT * FROM test_union_where.csv WHERE value < 25 "
                       "UNION "
                       "SELECT * FROM test_union_where.csv WHERE value > 25";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    printf("  UNION with WHERE: %d rows\n", result->row_count);
    assert(result->row_count == 4); // id 1,2 (< 25) and id 3,4 (> 25)
    csv_print_table(result, 10);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_union_where.csv");
    printf("  PASSED\n\n");
}

void test_multiple_unions() {
    printf("Test: Multiple UNIONs...\n");
    
    FILE* f1 = fopen("test_multi_a.csv", "w");
    fprintf(f1, "num\n1\n");
    fclose(f1);
    
    FILE* f2 = fopen("test_multi_b.csv", "w");
    fprintf(f2, "num\n2\n");
    fclose(f2);
    
    FILE* f3 = fopen("test_multi_c.csv", "w");
    fprintf(f3, "num\n3\n");
    fclose(f3);
    
    const char* query = "SELECT * FROM test_multi_a.csv "
                       "UNION SELECT * FROM test_multi_b.csv "
                       "UNION SELECT * FROM test_multi_c.csv";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 3); // 1, 2, 3
    printf("  Multiple UNIONs: %d rows\n", result->row_count);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_multi_a.csv");
    remove("test_multi_b.csv");
    remove("test_multi_c.csv");
    printf("  PASSED\n\n");
}

void test_union_different_columns() {
    printf("Test: UNION with incompatible columns (should fail)...\n");
    
    FILE* f1 = fopen("test_diff_a.csv", "w");
    fprintf(f1, "id,name\n1,Alice\n");
    fclose(f1);
    
    FILE* f2 = fopen("test_diff_b.csv", "w");
    fprintf(f2, "id\n1\n");  // different number of columns
    fclose(f2);
    
    const char* query = "SELECT * FROM test_diff_a.csv UNION SELECT * FROM test_diff_b.csv";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result == NULL);  // should fail due to column mismatch
    printf("  Correctly rejected incompatible columns\n");
    
    releaseNode(ast);
    
    remove("test_diff_a.csv");
    remove("test_diff_b.csv");
    printf("  PASSED\n\n");
}

void test_intersect_no_common() {
    printf("Test: INTERSECT with no common rows...\n");
    
    FILE* f1 = fopen("test_intersect_none_a.csv", "w");
    fprintf(f1, "val\n1\n2\n");
    fclose(f1);
    
    FILE* f2 = fopen("test_intersect_none_b.csv", "w");
    fprintf(f2, "val\n3\n4\n");
    fclose(f2);
    
    const char* query = "SELECT * FROM test_intersect_none_a.csv INTERSECT SELECT * FROM test_intersect_none_b.csv";
    ASTNode* ast = parse(query);
    assert(ast != NULL);
    
    ResultSet* result = evaluate_query(ast);
    assert(result != NULL);
    assert(result->row_count == 0); // no common rows
    printf("  INTERSECT with no common rows: %d rows\n", result->row_count);
    
    csv_free(result);
    releaseNode(ast);
    
    remove("test_intersect_none_a.csv");
    remove("test_intersect_none_b.csv");
    printf("  PASSED\n\n");
}

int main() {
    printf("=== Set Operations Tests (UNION, INTERSECT, EXCEPT) ===\n\n");
    
    test_union();
    test_union_all();
    test_intersect();
    test_except();
    test_union_with_where();
    test_multiple_unions();
    test_union_different_columns();
    test_intersect_no_common();
    
    printf("=== All set operation tests passed! ===\n");
    return 0;
}
