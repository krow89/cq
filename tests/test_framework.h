#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// test statistics
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// ANSI color codes
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_RESET "\033[0m"

// test assertion macros
#define TEST_START(name) \
    do { \
        tests_run++; \
        printf("Testing: %s ... ", name); \
        fflush(stdout); \
    } while(0)

#define TEST_PASS() \
    do { \
        tests_passed++; \
        printf(COLOR_GREEN "PASS" COLOR_RESET "\n"); \
    } while(0)

#define TEST_FAIL(msg, ...) \
    do { \
        tests_failed++; \
        printf(COLOR_RED "FAIL" COLOR_RESET "\n"); \
        printf("  " msg "\n", ##__VA_ARGS__); \
    } while(0)

#define ASSERT_EQUAL(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            TEST_FAIL("Expected %d, got %d", (int)(expected), (int)(actual)); \
            return; \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            TEST_FAIL("Condition failed: %s", #condition); \
            return; \
        } \
    } while(0)

#define ASSERT_NOT_NULL(ptr) \
    do { \
        if ((ptr) == NULL) { \
            TEST_FAIL("Expected non-NULL pointer"); \
            return; \
        } \
    } while(0)

#define ASSERT_NULL(ptr) \
    do { \
        if ((ptr) != NULL) { \
            TEST_FAIL("Expected NULL pointer"); \
            return; \
        } \
    } while(0)

// print test summary
static void print_test_summary(void) {
    printf("\n");
    printf("=== Test Summary ===\n");
    printf("Total:  %d\n", tests_run);
    printf(COLOR_GREEN "Passed: %d" COLOR_RESET "\n", tests_passed);
    if (tests_failed > 0) {
        printf(COLOR_RED "Failed: %d" COLOR_RESET "\n", tests_failed);
    } else {
        printf("Failed: %d\n", tests_failed);
    }
    printf("\n");
    
    if (tests_failed == 0) {
        printf(COLOR_GREEN "All tests passed!" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "Some tests failed." COLOR_RESET "\n");
    }
}

#endif
