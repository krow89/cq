#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "tokenizer.h"

/* test helper to check token properties */
void assert_token(Token* tokens, int index, TokenType expected_type, const char* expected_value) {
    assert(tokens[index].type == expected_type);
    assert(strcmp(tokens[index].value, expected_value) == 0);
}

/* test 1: Simple SELECT statement */
void test_simple_select() {
    printf("Running test_simple_select...\n");
    
    const char* sql = "SELECT name FROM users";
    int count = 0;
    Token* tokens = tokenize(sql, &count);
    
    assert(tokens != NULL);
    assert(count == 5); // SELECT, name, FROM, users, EOF
    
    assert_token(tokens, 0, TOKEN_TYPE_KEYWORD, "SELECT");
    assert_token(tokens, 1, TOKEN_TYPE_IDENTIFIER, "name");
    assert_token(tokens, 2, TOKEN_TYPE_KEYWORD, "FROM");
    assert_token(tokens, 3, TOKEN_TYPE_IDENTIFIER, "users");
    assert_token(tokens, 4, TOKEN_TYPE_EOF, "");
    
    freeTokens(tokens, count);
    printf("✓ test_simple_select passed\n\n");
}

/* test 2: WHERE clause with operators */
void test_where_clause() {
    printf("Running test_where_clause...\n");
    
    const char* sql = "WHERE age > 30 AND active = 1";
    int count = 0;
    Token* tokens = tokenize(sql, &count);
    
    assert(tokens != NULL);
    assert(count == 9); // WHERE, age, >, 30, AND, active, =, 1, EOF
    
    assert_token(tokens, 0, TOKEN_TYPE_KEYWORD, "WHERE");
    assert_token(tokens, 1, TOKEN_TYPE_IDENTIFIER, "age");
    assert_token(tokens, 2, TOKEN_TYPE_OPERATOR, ">");
    assert_token(tokens, 3, TOKEN_TYPE_LITERAL, "30");
    assert_token(tokens, 4, TOKEN_TYPE_KEYWORD, "AND");
    assert_token(tokens, 5, TOKEN_TYPE_IDENTIFIER, "active");
    assert_token(tokens, 6, TOKEN_TYPE_OPERATOR, "=");
    assert_token(tokens, 7, TOKEN_TYPE_LITERAL, "1");
    assert_token(tokens, 8, TOKEN_TYPE_EOF, "");
    
    freeTokens(tokens, count);
    printf("✓ test_where_clause passed\n\n");
}

/* test 3: String literals */
void test_string_literals() {
    printf("Running test_string_literals...\n");
    
    const char* sql = "WHERE name = 'John Doe' OR role = \"admin\"";
    int count = 0;
    Token* tokens = tokenize(sql, &count);
    
    assert(tokens != NULL);
    
    // find the string literals
    int found_single_quote = 0, found_double_quote = 0;
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOKEN_TYPE_LITERAL) {
            if (strcmp(tokens[i].value, "John Doe") == 0) found_single_quote = 1;
            if (strcmp(tokens[i].value, "admin") == 0) found_double_quote = 1;
        }
    }
    
    assert(found_single_quote);
    assert(found_double_quote);
    
    freeTokens(tokens, count);
    printf("✓ test_string_literals passed\n\n");
}

/* test 4: Punctuation and function calls */
void test_punctuation_and_functions() {
    printf("Running test_punctuation_and_functions...\n");
    
    const char* sql = "SELECT ROUND(height), COUNT(*)";
    int count = 0;
    Token* tokens = tokenize(sql, &count);
    
    assert(tokens != NULL);
    
    assert_token(tokens, 0, TOKEN_TYPE_KEYWORD, "SELECT");
    assert_token(tokens, 1, TOKEN_TYPE_IDENTIFIER, "ROUND");
    assert_token(tokens, 2, TOKEN_TYPE_PUNCTUATION, "(");
    assert_token(tokens, 3, TOKEN_TYPE_IDENTIFIER, "height");
    assert_token(tokens, 4, TOKEN_TYPE_PUNCTUATION, ")");
    assert_token(tokens, 5, TOKEN_TYPE_PUNCTUATION, ",");
    
    freeTokens(tokens, count);
    printf("✓ test_punctuation_and_functions passed\n\n");
}

/* test 5: Complex query from main */
void test_complex_query() {
    printf("Running test_complex_query...\n");
    
    const char* sql = "SELECT role, name, ROUND(height) AS rounded_height WHERE (age > 30 OR age < 10) AND active = 1 AND role = IN ('user', 'operator') GROUP BY role ORDER BY height DESC";
    int count = 0;
    Token* tokens = tokenize(sql, &count);
    
    assert(tokens != NULL);
    assert(count > 30); // should have many tokens
    
    // check it starts correctly
    assert_token(tokens, 0, TOKEN_TYPE_KEYWORD, "SELECT");
    assert_token(tokens, 1, TOKEN_TYPE_IDENTIFIER, "role");
    assert_token(tokens, 2, TOKEN_TYPE_PUNCTUATION, ",");
    
    // check it ends with EOF
    assert_token(tokens, count - 1, TOKEN_TYPE_EOF, "");
    
    printf("Token count: %d\n", count);
    printTokens(tokens, count);
    
    freeTokens(tokens, count);
    printf("✓ test_complex_query passed\n\n");
}

/* test 6: Two-character operators */
void test_two_char_operators() {
    printf("Running test_two_char_operators...\n");
    
    const char* sql = "WHERE a >= 5 AND b <= 10 AND c != 0";
    int count = 0;
    Token* tokens = tokenize(sql, &count);
    
    assert(tokens != NULL);
    
    // find the operators
    int found_ge = 0, found_le = 0, found_ne = 0;
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOKEN_TYPE_OPERATOR) {
            if (strcmp(tokens[i].value, ">=") == 0) found_ge = 1;
            if (strcmp(tokens[i].value, "<=") == 0) found_le = 1;
            if (strcmp(tokens[i].value, "!=") == 0) found_ne = 1;
        }
    }
    
    assert(found_ge);
    assert(found_le);
    assert(found_ne);
    
    freeTokens(tokens, count);
    printf("✓ test_two_char_operators passed\n\n");
}

/* test 7: Edge cases */
void test_edge_cases() {
    printf("Running test_edge_cases...\n");
    
    // empty string
    int count = 0;
    Token* tokens = tokenize("", &count);
    assert(tokens != NULL);
    assert(count == 1); // just EOF
    assert_token(tokens, 0, TOKEN_TYPE_EOF, "");
    freeTokens(tokens, count);
    
    // only whitespace
    tokens = tokenize("   \t\n  ", &count);
    assert(tokens != NULL);
    assert(count == 1); // just EOF
    freeTokens(tokens, count);
    
    // NULL input
    tokens = tokenize(NULL, &count);
    assert(tokens == NULL);
    assert(count == 0);
    
    printf("✓ test_edge_cases passed\n\n");
}

/* test 8: Qualified identifiers with dot */
void test_qualified_identifiers() {
    printf("Running test_qualified_identifiers...\n");
    
    const char* sql = "SELECT f1.name, f2.age FROM table1";
    int count = 0;
    Token* tokens = tokenize(sql, &count);
    
    assert(tokens != NULL);
    // SELECT f1 . name , f2 . age FROM table1 EOF
    // should have: SELECT, f1, ., name, ,, f2, ., age, FROM, table1, EOF
    
    assert_token(tokens, 0, TOKEN_TYPE_KEYWORD, "SELECT");
    assert_token(tokens, 1, TOKEN_TYPE_IDENTIFIER, "f1");
    assert_token(tokens, 2, TOKEN_TYPE_PUNCTUATION, ".");
    assert_token(tokens, 3, TOKEN_TYPE_IDENTIFIER, "name");
    assert_token(tokens, 4, TOKEN_TYPE_PUNCTUATION, ",");
    assert_token(tokens, 5, TOKEN_TYPE_IDENTIFIER, "f2");
    assert_token(tokens, 6, TOKEN_TYPE_PUNCTUATION, ".");
    assert_token(tokens, 7, TOKEN_TYPE_IDENTIFIER, "age");
    
    freeTokens(tokens, count);
    printf("✓ test_qualified_identifiers passed\n\n");
}

/* test 9: Decimal numbers with dot */
void test_decimal_numbers() {
    printf("Running test_decimal_numbers...\n");
    
    const char* sql = "WHERE height > 175.5 AND price = 99.99";
    int count = 0;
    Token* tokens = tokenize(sql, &count);
    
    assert(tokens != NULL);
    
    // find the decimal literals
    int found_175 = 0, found_99 = 0;
    for (int i = 0; i < count; i++) {
        if (tokens[i].type == TOKEN_TYPE_LITERAL) {
            if (strcmp(tokens[i].value, "175.5") == 0) found_175 = 1;
            if (strcmp(tokens[i].value, "99.99") == 0) found_99 = 1;
        }
    }
    
    assert(found_175);
    assert(found_99);
    
    freeTokens(tokens, count);
    printf("✓ test_decimal_numbers passed\n\n");
}

int main(void) {
    printf("=== Tokenizer Test Suite ===\n\n");
    
    test_simple_select();
    test_where_clause();
    test_string_literals();
    test_punctuation_and_functions();
    test_two_char_operators();
    test_edge_cases();
    test_qualified_identifiers();
    test_decimal_numbers();
    test_complex_query();
    
    printf("=== All tests passed! ===\n");
    return 0;
}
