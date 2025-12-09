#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "../include/csv_reader.h"

// Function to get current time in microseconds
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000.0) + (tv.tv_usec / 1000.0);
}

int main(void) {
    printf("=== CSV Load Performance Test ===\n\n");
    
    const char* filename = "data/bigdata.csv";
    CsvConfig config = {
        .delimiter = ',',
        .quote = '"',
        .has_header = true
    };
    
    printf("Testing file: %s\n\n", filename);
    
    // Measure total load time (includes file reading + parsing)
    double start_total = get_time_ms();
    CsvTable* table = csv_load(filename, config);
    double end_total = get_time_ms();
    
    if (!table) {
        fprintf(stderr, "Error: Failed to load CSV file\n");
        return 1;
    }
    
    double total_time = end_total - start_total;
    
    // Display results
    printf("Results:\n");
    printf("--------\n");
    printf("Rows loaded:      %d\n", table->row_count);
    printf("Columns:          %d\n", table->column_count);
    printf("Total time:       %.2f ms\n", total_time);
    printf("Time per row:     %.4f ms\n", total_time / table->row_count);
    printf("Rows per second:  %.0f\n", (table->row_count / total_time) * 1000.0);
    
    printf("\nBreakdown:\n");
    printf("  File I/O + CSV parsing: %.2f ms (100%%)\n", total_time);
    
    // Estimate memory usage
    size_t memory_used = 0;
    memory_used += sizeof(CsvTable);
    memory_used += sizeof(Column) * table->column_count;
    memory_used += sizeof(Row) * table->row_capacity;
    
    for (int i = 0; i < table->row_count; i++) {
        memory_used += sizeof(Value) * table->rows[i].column_count;
        for (int j = 0; j < table->rows[i].column_count; j++) {
            if (table->rows[i].values[j].type == VALUE_TYPE_STRING && 
                table->rows[i].values[j].string_value) {
                memory_used += strlen(table->rows[i].values[j].string_value) + 1;
            }
        }
    }
    
    printf("\nMemory usage (approximate):\n");
    printf("  Total: %.2f MB\n", memory_used / (1024.0 * 1024.0));
    printf("  Per row: %.2f KB\n", (memory_used / (double)table->row_count) / 1024.0);
    
    // Cleanup
    csv_free(table);
    
    printf("\n=== Test completed successfully ===\n");
    
    return 0;
}
