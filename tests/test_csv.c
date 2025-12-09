#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "csv_reader.h"

void test_csv_load() {
    printf("Running test_csv_load...\n");
    
    CsvConfig config = csv_config_default();
    CsvTable* table = csv_load("data/test_data.csv", config);
    
    assert(table != NULL);
    assert(table->row_count == 7);
    assert(table->column_count > 0);  // infer from header, not hardcoded
    
    // check some expected columns exist
    assert(csv_get_column_index(table, "name") >= 0);
    assert(csv_get_column_index(table, "age") >= 0);
    assert(csv_get_column_index(table, "role") >= 0);
    
    csv_free(table);
    printf("✓ test_csv_load passed\n\n");
}

void test_csv_values() {
    printf("Running test_csv_values...\n");
    
    CsvConfig config = csv_config_default();
    CsvTable* table = csv_load("data/test_data.csv", config);
    
    // check first row
    Value* name = csv_get_value_by_name(table, 0, "name");
    assert(name != NULL);
    assert(name->type == VALUE_TYPE_STRING);
    
    Value* age = csv_get_value_by_name(table, 0, "age");
    assert(age != NULL);
    assert(age->type == VALUE_TYPE_INTEGER);
    assert(age->int_value == 25);
    
    Value* height = csv_get_value_by_name(table, 0, "height");
    assert(height != NULL);
    assert(height->type == VALUE_TYPE_DOUBLE);
    
    csv_free(table);
    printf("✓ test_csv_values passed\n\n");
}

void test_csv_no_header() {
    printf("Running test_csv_no_header...\n");
    
    CsvConfig config = csv_config_default();
    config.has_header = false;
    
    CsvTable* table = csv_load("data/test_data.csv", config);
    
    assert(table != NULL);
    assert(table->row_count == 8); // all lines including first (7 data + 1 header treated as data)
    
    // check generated column names
    assert(csv_get_column_index(table, "$0") == 0);
    assert(csv_get_column_index(table, "$1") == 1);
    
    csv_free(table);
    printf("✓ test_csv_no_header passed\n\n");
}

void test_csv_print() {
    printf("Running test_csv_print...\n");
    
    CsvConfig config = csv_config_default();
    CsvTable* table = csv_load("data/test_data.csv", config);
    
    printf("\n");
    csv_print_table(table, 10);
    printf("\n");
    
    csv_free(table);
    printf("✓ test_csv_print passed\n\n");
}

int main(void) {
    printf("=== CSV Reader Test Suite ===\n\n");
    
    test_csv_load();
    test_csv_values();
    test_csv_no_header();
    test_csv_print();
    
    printf("=== All CSV tests passed! ===\n");
    return 0;
}
