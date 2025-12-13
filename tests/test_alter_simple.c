#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "test_framework.h"
#include "parser.h"
#include "evaluator.h"
#include "csv_reader.h"

void test_add_only() {
    TEST_START("ADD COLUMN only");
    
    const char* test_file = "data/test_simple_add.csv";
    unlink(test_file);
    
    FILE* f = fopen(test_file, "w");
    fprintf(f, "id,name\n");
    fprintf(f, "1,Alice\n");
    fclose(f);
    
    const char* sql = "ALTER TABLE 'data/test_simple_add.csv' ADD COLUMN email";
    ASTNode* ast = parse(sql);
    ResultSet* result = evaluate_query(ast);
    
    printf("Result: %p\n", result);
    if (result) {
        printf("Row count: %d\n", result->row_count);
    }
    
    csv_free(result);
    releaseNode(ast);
    unlink(test_file);
    
    TEST_PASS();
}

int main() {
    test_add_only();
    print_test_summary();
    return 0;
}
