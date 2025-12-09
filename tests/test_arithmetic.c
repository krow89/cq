#include "test_framework.h"
#include "test_helpers.h"

void test_simple_addition(void) {
    TEST_START("Simple addition");
    int count = execute_query_count("SELECT age + 10 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_simple_subtraction(void) {
    TEST_START("Simple subtraction");
    int count = execute_query_count("SELECT age - 5 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_simple_multiplication(void) {
    TEST_START("Simple multiplication");
    int count = execute_query_count("SELECT age * 2 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_simple_division(void) {
    TEST_START("Simple division");
    int count = execute_query_count("SELECT age / 10 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_multiplication_before_addition(void) {
    TEST_START("Multiplication before addition");
    int count = execute_query_count("SELECT 2 + 3 * 4 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_division_before_subtraction(void) {
    TEST_START("Division before subtraction");
    int count = execute_query_count("SELECT 20 - 10 / 2 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_parentheses_change_precedence(void) {
    TEST_START("Parentheses change precedence");
    int count = execute_query_count("SELECT (2 + 3) * 4 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_nested_parentheses(void) {
    TEST_START("Nested parentheses");
    int count = execute_query_count("SELECT ((age + 5) * 2) - 10 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_complex_expression(void) {
    TEST_START("Complex expression");
    int count = execute_query_count("SELECT (age + 10) * 2 / 5 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_arithmetic_in_where_simple(void) {
    TEST_START("Arithmetic in WHERE - simple");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age * 2 > 60;");
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_arithmetic_in_where_with_parentheses(void) {
    TEST_START("Arithmetic in WHERE - with parentheses");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE (age + 5) * 2 > 70;");
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_arithmetic_on_both_sides(void) {
    TEST_START("Arithmetic on both sides");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age * 2 > 30 * 2;");
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_division_in_where(void) {
    TEST_START("Division in WHERE");
    int count = execute_query_count("SELECT name FROM 'data/test_data.csv' WHERE age / 10 > 3;");
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_column_with_multiple_operations(void) {
    TEST_START("Column with multiple operations");
    int count = execute_query_count("SELECT name, age, (age + 10) * 2 - 5 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_multiple_arithmetic_columns(void) {
    TEST_START("Multiple arithmetic columns");
    int count = execute_query_count("SELECT age + 1, age * 2, age - 5 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_constant_expression(void) {
    TEST_START("Constant expression");
    int count = execute_query_count("SELECT 1 + 1 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_complex_constant(void) {
    TEST_START("Complex constant");
    int count = execute_query_count("SELECT (10 + 5) * 2 - 8 / 4 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_double_values_in_arithmetic(void) {
    TEST_START("Double values in arithmetic");
    int count = execute_query_count("SELECT height * 2 FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_mixed_integer_and_double(void) {
    TEST_START("Mixed integer and double");
    int count = execute_query_count("SELECT age + height FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_arithmetic_with_where_using_same_expression(void) {
    TEST_START("Arithmetic with WHERE using same expression");
    int count = execute_query_count("SELECT name, age * 2 FROM 'data/test_data.csv' WHERE age * 2 > 50;");
    ASSERT_EQUAL(5, count);
    TEST_PASS();
}

void test_expression_with_alias(void) {
    TEST_START("Expression with alias");
    int count = execute_query_count("SELECT age * 2 AS double_age FROM 'data/test_data.csv';");
    ASSERT_EQUAL(7, count);
    TEST_PASS();
}

void test_complex_expression_with_alias(void) {
    TEST_START("Complex expression with alias");
    int count = execute_query_count("SELECT (age + 5) * 2 AS modified FROM 'data/test_data.csv' WHERE age > 25;");
    ASSERT_EQUAL(5, count);
    TEST_PASS();
}

int main(void) {
    printf("=== Arithmetic Expression Tests ===\n\n");
    
    // run all tests
    test_simple_addition();
    test_simple_subtraction();
    test_simple_multiplication();
    test_simple_division();
    test_multiplication_before_addition();
    test_division_before_subtraction();
    test_parentheses_change_precedence();
    test_nested_parentheses();
    test_complex_expression();
    test_arithmetic_in_where_simple();
    test_arithmetic_in_where_with_parentheses();
    test_arithmetic_on_both_sides();
    test_division_in_where();
    test_column_with_multiple_operations();
    test_multiple_arithmetic_columns();
    test_constant_expression();
    test_complex_constant();
    test_double_values_in_arithmetic();
    test_mixed_integer_and_double();
    test_arithmetic_with_where_using_same_expression();
    test_expression_with_alias();
    test_complex_expression_with_alias();
    
    print_test_summary();
    
    return (tests_failed == 0) ? 0 : 1;
}
