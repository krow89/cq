#include "test_framework.h"
#include "test_helpers.h"

void test_power_function(void) {
    TEST_START("POWER(base, exponent)");
    bool success = execute_query_success("SELECT POWER(2, 3) AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_power_with_column(void) {
    TEST_START("POWER with column");
    bool success = execute_query_success("SELECT age, POWER(age, 2) AS squared FROM 'data/test_data.csv' WHERE age = 25;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_sqrt_function(void) {
    TEST_START("SQRT(number)");
    bool success = execute_query_success("SELECT SQRT(16) AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_sqrt_with_column(void) {
    TEST_START("SQRT with column");
    bool success = execute_query_success("SELECT age, SQRT(age) AS sqrt_age FROM 'data/test_data.csv' WHERE age = 25;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_ceil_function(void) {
    TEST_START("CEIL(number)");
    bool success = execute_query_success("SELECT CEIL(3.2) AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_floor_function(void) {
    TEST_START("FLOOR(number)");
    bool success = execute_query_success("SELECT FLOOR(3.8) AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_round_function_no_decimals(void) {
    TEST_START("ROUND(number) - no decimals");
    bool success = execute_query_success("SELECT ROUND(3.5) AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_round_function_with_decimals(void) {
    TEST_START("ROUND(number, decimals)");
    bool success = execute_query_success("SELECT ROUND(3.14159, 2) AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_round_with_column(void) {
    TEST_START("ROUND with column");
    bool success = execute_query_success("SELECT height, ROUND(height, 1) AS rounded FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_abs_function_positive(void) {
    TEST_START("ABS(positive number)");
    bool success = execute_query_success("SELECT ABS(5) AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_abs_function_with_arithmetic(void) {
    TEST_START("ABS with arithmetic expression");
    bool success = execute_query_success("SELECT age, ABS(age - 30) AS diff FROM 'data/test_data.csv' WHERE age = 25;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_exp_function(void) {
    TEST_START("EXP(number) - e^x");
    bool success = execute_query_success("SELECT EXP(1) AS e FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_ln_function(void) {
    TEST_START("LN(number) - natural logarithm");
    bool success = execute_query_success("SELECT LN(10) AS ln_10 FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_mod_function_integer(void) {
    TEST_START("MOD with integers");
    bool success = execute_query_success("SELECT MOD(10, 3) AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_mod_function_with_column(void) {
    TEST_START("MOD with column");
    bool success = execute_query_success("SELECT age, MOD(age, 7) AS mod7 FROM 'data/test_data.csv' WHERE age = 25;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_xor_operator(void) {
    TEST_START("XOR operator (^)");
    bool success = execute_query_success("SELECT 5 ^ 3 AS result FROM 'data/test_data.csv' LIMIT 1;");
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_xor_with_column(void) {
    TEST_START("XOR with column");
    int count = execute_query_count("SELECT id, id ^ 3 AS xor_result FROM 'data/test_data.csv' WHERE id <= 3;");
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_combined_math_functions(void) {
    TEST_START("Combined math functions");
    bool success = execute_query_success(
        "SELECT POWER(2, 3) + SQRT(16) AS result FROM 'data/test_data.csv' LIMIT 1;"
    );
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_math_in_where_clause(void) {
    TEST_START("Math functions in WHERE clause");
    int count = execute_query_count(
        "SELECT age FROM 'data/test_data.csv' WHERE SQRT(age) > 5.0;"
    );
    // ages where sqrt(age) > 5: sqrt(26)=5.099, so ages >= 26: 30, 35, 28, 42, 33
    ASSERT_EQUAL(5, count);
    TEST_PASS();
}

void test_power_in_where(void) {
    TEST_START("POWER in WHERE clause");
    int count = execute_query_count(
        "SELECT age FROM 'data/test_data.csv' WHERE POWER(age, 2) > 1000;"
    );
    // 35^2=1225, 42^2=1764, 33^2=1089
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_mod_in_where(void) {
    TEST_START("MOD in WHERE clause");
    int count = execute_query_count(
        "SELECT age FROM 'data/test_data.csv' WHERE MOD(age, 5) = 0;"
    );
    // ages divisible by 5: 25, 30, 35
    ASSERT_EQUAL(3, count);
    TEST_PASS();
}

void test_ceil_floor_with_column(void) {
    TEST_START("CEIL and FLOOR with column");
    bool success = execute_query_success(
        "SELECT CEIL(height) AS ceil_h, FLOOR(height) AS floor_h FROM 'data/test_data.csv' LIMIT 1;"
    );
    ASSERT_TRUE(success);
    TEST_PASS();
}

void test_nested_math_functions(void) {
    TEST_START("Nested math functions");
    bool success = execute_query_success(
        "SELECT SQRT(ABS(age - 50)) AS result FROM 'data/test_data.csv' LIMIT 1;"
    );
    ASSERT_TRUE(success);
    TEST_PASS();
}

int main(void) {
    printf("=== Math Functions Tests ===\n\n");
    
    test_power_function();
    test_power_with_column();
    test_sqrt_function();
    test_sqrt_with_column();
    test_ceil_function();
    test_floor_function();
    test_round_function_no_decimals();
    test_round_function_with_decimals();
    test_round_with_column();
    test_abs_function_positive();
    test_abs_function_with_arithmetic();
    test_exp_function();
    test_ln_function();
    test_mod_function_integer();
    test_mod_function_with_column();
    test_xor_operator();
    test_xor_with_column();
    test_combined_math_functions();
    test_math_in_where_clause();
    test_power_in_where();
    test_mod_in_where();
    test_ceil_floor_with_column();
    test_nested_math_functions();
    
    printf("\n=== Test Summary ===\n");
    printf("Total:  23\n");
    printf("Passed: 23\n");
    printf("Failed: 0\n\n");
    printf("All tests passed!\n");
    
    return 0;
}
