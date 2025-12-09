#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/parser.h"
#include "../include/evaluator.h"
#include "../include/csv_reader.h"

// execute a query and return the number of result rows
int execute_query_count(const char* query_str) {
    ASTNode* ast = parse(query_str);
    if (!ast) {
        return -1;
    }
    
    ResultSet* result = evaluate_query(ast);
    if (!result) {
        releaseNode(ast);
        return -1;
    }
    
    int count = result->row_count;
    
    csv_free(result);
    releaseNode(ast);
    
    return count;
}

// execute a query and return whether it succeeded
bool execute_query_success(const char* query_str) {
    ASTNode* ast = parse(query_str);
    if (!ast) {
        return false;
    }
    
    ResultSet* result = evaluate_query(ast);
    bool success = (result != NULL);
    
    if (result) {
        csv_free(result);
    }
    releaseNode(ast);
    
    return success;
}

#endif
