#include "test_framework.h"
#include "test_helpers.h"

void test_modulo_simple(void) {
    TEST_START("Modulo - simple");
    int count = execute_query_count("SELECT age % 10 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_modulo_in_where(void) {
    TEST_START("Modulo in WHERE");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age % 10 = 5;");
    ASSERT_EQUAL(2, count);
    TEST_PASS();
}

void test_modulo_even_numbers(void) {
    TEST_START("Modulo - even numbers");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age % 2 = 0;");
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_modulo_odd_numbers(void) {
    TEST_START("Modulo - odd numbers");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age % 2 = 1;");
    ASSERT_EQUAL(4, count);
    TEST_PASS();
}

void test_bitwise_and_simple(void) {
    TEST_START("Bitwise AND - simple");
    int count = execute_query_count("SELECT age & 15 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_bitwise_and_in_where(void) {
    TEST_START("Bitwise AND in WHERE");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age & 1 = 1;");
    ASSERT_EQUAL(4, count);
    TEST_PASS();
}

void test_bitwise_and_check_bit(void) {
    TEST_START("Bitwise AND - check bit");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE (age & 16) > 0;");
    ASSERT_EQUAL(4, count);
    TEST_PASS();
}

void test_bitwise_or_simple(void) {
    TEST_START("Bitwise OR - simple");
    int count = execute_query_count("SELECT age | 1 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_bitwise_or_in_where(void) {
    TEST_START("Bitwise OR in WHERE");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE (age | 1) > 30;");
    ASSERT_EQUAL(4, count);
    TEST_PASS();
}

void test_bitwise_combined(void) {
    TEST_START("Bitwise combined");
    int count = execute_query_count("SELECT age, age & 15, age | 1 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_bitwise_with_arithmetic(void) {
    TEST_START("Bitwise with arithmetic");
    int count = execute_query_count("SELECT age, (age & 15) + 10 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_not_with_comparison(void) {
    TEST_START("NOT with comparison");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE NOT age > 30;");
    ASSERT_EQUAL(4, count);
    TEST_PASS();
}

void test_not_with_equality(void) {
    TEST_START("NOT with equality");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE NOT age = 25;");
    ASSERT_EQUAL(6, count);
    TEST_PASS();
}

void test_not_with_complex_condition(void) {
    TEST_START("NOT with complex condition");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE NOT (age > 20 AND age < 30);");
    ASSERT_EQUAL(5, count);
    TEST_PASS();
}

void test_not_in_with_list(void) {
    TEST_START("NOT IN with list");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age NOT IN (25, 30, 35);");
    ASSERT_EQUAL(4, count);
    TEST_PASS();
}

void test_not_in_with_more_values(void) {
    TEST_START("NOT IN with more values");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age NOT IN (19, 25, 30);");
    ASSERT_EQUAL(4, count);
    TEST_PASS();
}

void test_modulo_with_arithmetic(void) {
    TEST_START("Modulo with arithmetic");
    int count = execute_query_count("SELECT age, (age % 10) * 2 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_all_operators_combined(void) {
    TEST_START("All operators combined");
    int count = execute_query_count("SELECT age, age % 5, age & 7, age | 1 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_precedence_modulo_and_add(void) {
    TEST_START("Precedence - modulo and add");
    int count = execute_query_count("SELECT 10 + 7 % 3 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_precedence_bitwise_lower_than_arithmetic(void) {
    TEST_START("Precedence - bitwise lower than arithmetic");
    int count = execute_query_count("SELECT 5 + 3 & 4 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_not_and_and_combined(void) {
    TEST_START("NOT and AND combined");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE NOT (age < 25 OR age > 35);");
    ASSERT_EQUAL(5, count);
    TEST_PASS();
}

void test_multiple_not(void) {
    TEST_START("Multiple NOT");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE NOT NOT age > 30;");
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_modulo_in_complex_expression(void) {
    TEST_START("Modulo in complex expression");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE (age % 10) + (age / 10) > 5;");
    ASSERT_EQUAL(6, count);
    TEST_PASS();
}

int main(void) {
    printf("=== Extended Operators Tests ===\n\n");
    
    // run all tests
    test_modulo_simple();
    test_modulo_in_where();
    test_modulo_even_numbers();
    test_modulo_odd_numbers();
    test_bitwise_and_simple();
    test_bitwise_and_in_where();
    test_bitwise_and_check_bit();
    test_bitwise_or_simple();
    test_bitwise_or_in_where();
    test_bitwise_combined();
    test_bitwise_with_arithmetic();
    test_not_with_comparison();
    test_not_with_equality();
    test_not_with_complex_condition();
    test_not_in_with_list();
    test_not_in_with_more_values();
    test_modulo_with_arithmetic();
    test_all_operators_combined();
    test_precedence_modulo_and_add();
    test_precedence_bitwise_lower_than_arithmetic();
    test_not_and_and_combined();
    test_multiple_not();
    test_modulo_in_complex_expression();
    
    print_test_summary();
    
    return (tests_failed == 0) ? 0 : 1;
}
