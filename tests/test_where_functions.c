#include "test_framework.h"
#include "test_helpers.h"

void test_length_in_where(void) {
    TEST_START("WHERE LENGTH(name) > 5");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE LENGTH(name) > 5;");
    ASSERT_EQUAL(1, count);  // Charlie has 7 chars
    TEST_PASS();
}

void test_upper_in_where(void) {
    TEST_START("WHERE UPPER(role) = 'ADMIN'");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE UPPER(role) = 'ADMIN';");
    ASSERT_EQUAL(2, count);  // Alice and Eve are admins
    TEST_PASS();
}

void test_lower_in_where(void) {
    TEST_START("WHERE LOWER(name) = 'bob'");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE LOWER(name) = 'bob';");
    ASSERT_EQUAL(1, count);
    TEST_PASS();
}

void test_substring_in_where(void) {
    TEST_START("WHERE SUBSTRING(name, 1, 1) = 'A'");
    bool success = execute_query_success("SELECT name FROM 'data/test_data.csv' WHERE SUBSTRING(name, 1, 1) = 'A';");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_nested_functions_in_where(void) {
    TEST_START("WHERE LENGTH(CONCAT(name, role)) > 10");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE LENGTH(CONCAT(name, role)) > 10;");
    ASSERT_EQUAL(2, count);  // Charlie+moderator=16, Grace+moderator=14
    TEST_PASS();
}

void test_replace_in_where(void) {
    TEST_START("WHERE REPLACE(role, 'admin', 'ADMIN') = 'ADMIN'");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE REPLACE(role, 'admin', 'ADMIN') = 'ADMIN';");
    ASSERT_EQUAL(2, count);
    TEST_PASS();
}

void test_function_with_and(void) {
    TEST_START("WHERE LENGTH(name) > 4 AND UPPER(role) = 'USER'");
    bool success = execute_query_success("SELECT name FROM 'data/test_data.csv' WHERE LENGTH(name) > 4 AND UPPER(role) = 'USER';");
    ASSERT_TRUE(success);  // Frank has 5 chars and is user
    TEST_PASS();
}

void test_function_with_or(void) {
    TEST_START("WHERE LENGTH(name) = 3 OR UPPER(role) = 'ADMIN'");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE LENGTH(name) = 3 OR UPPER(role) = 'ADMIN';");
    ASSERT_EQUAL(3, count);  // Bob (3 chars), Alice (admin), Eve (both)
    TEST_PASS();
}

void test_coalesce_in_where(void) {
    TEST_START("WHERE COALESCE(role, 'unknown') = 'admin'");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE COALESCE(role, 'unknown') = 'admin';");
    ASSERT_EQUAL(2, count);
    TEST_PASS();
}

void test_complex_nested_function(void) {
    TEST_START("WHERE UPPER(SUBSTRING(name, 1, 3)) = 'BOB'");
    bool success = execute_query_success("SELECT name FROM 'data/test_data.csv' WHERE UPPER(SUBSTRING(name, 1, 3)) = 'BOB';");
    ASSERT_TRUE(success);
    TEST_PASS();
}

int main(void) {
    printf("=== Functions in WHERE Clause Tests ===\n\n");
    
    test_length_in_where();
    test_upper_in_where();
    test_lower_in_where();
    test_substring_in_where();
    test_nested_functions_in_where();
    test_replace_in_where();
    test_function_with_and();
    test_function_with_or();
    test_coalesce_in_where();
    test_complex_nested_function();
    
    print_test_summary();
    
    return (tests_failed == 0) ? 0 : 1;
}
