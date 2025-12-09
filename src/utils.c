#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "csv_reader.h"
#include "evaluator.h"
#include "utils.h"


/* function to skip whitespace characters in the input string
 * it returns a pointer to the next first non-whitespace character
 */
char* skipWhitespaces(char* str) {
    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
    return str;
}

void print_help(const char* program_name) {
    printf("Usage: %s [OPTIONS]\n", program_name);
    printf("\nOptions:\n");
    printf("  -h           Show this help message\n");
    printf("  -q <query>   SQL query to execute (required)\n");
    printf("  -o <file>    Write result as CSV to output file\n");
    printf("  -c           Print count of rows that match the query\n");
    printf("  -p           Print result as formatted table to stdout\n");
    printf("  -s <char>    Field separator for input CSV (default: ',')\n");
    printf("  -d <char>    Output delimiter for -o option (default: ',')\n");
    printf("\nExample:\n");
    printf("  %s -q \"SELECT name, age WHERE age > 30\" -p\n", program_name);
    printf("  %s -q \"SELECT * WHERE active = 1\" -o output.csv -c\n", program_name);
    printf("  %s -q \"SELECT * FROM data.tsv\" -s '\\t' -p\n", program_name);
}

/* Write ResultSet to CSV file */
void write_csv_file(const char* filename, ResultSet* result, char delimiter) {
    FILE* f = fopen(filename, "w");
    if (!f) {
        fprintf(stderr, "Error: Cannot open output file '%s'\n", filename);
        return;
    }
    
    // header
    for (int i = 0; i < result->column_count; i++) {
        if (i > 0) fprintf(f, "%c", delimiter);
        fprintf(f, "%s", result->columns[i].name);
    }
    fprintf(f, "\n");
    
    // rows
    for (int i = 0; i < result->row_count; i++) {
        Row* row = &result->rows[i];
        for (int j = 0; j < row->column_count; j++) {
            if (j > 0) fprintf(f, "%c", delimiter);
            
            Value* val = &row->values[j];
            switch (val->type) {
                case VALUE_TYPE_NULL:
                    break;
                case VALUE_TYPE_INTEGER:
                    fprintf(f, "%lld", val->int_value);
                    break;
                case VALUE_TYPE_DOUBLE:
                    fprintf(f, "%.2f", val->double_value);
                    break;
                case VALUE_TYPE_STRING: {
                    // check if string contains delimiter, newline, or quote char
                    bool needs_quoting = false;
                    if (val->string_value) {
                        for (const char* p = val->string_value; *p; p++) {
                            if (*p == delimiter || *p == '"' || *p == '\n' || *p == '\r') {
                                needs_quoting = true;
                                break;
                            }
                        }
                    }
                    
                    if (needs_quoting) {
                        fprintf(f, "\"");
                        // escape quotes by doubling them
                        for (const char* p = val->string_value; *p; p++) {
                            if (*p == '"') {
                                fprintf(f, "\"\"");
                            } else {
                                fprintf(f, "%c", *p);
                            }
                        }
                        fprintf(f, "\"");
                    } else {
                        fprintf(f, "%s", val->string_value ? val->string_value : "");
                    }
                    break;
                }
            }
        }
        fprintf(f, "\n");
    }
    
    fclose(f);
    printf("Result written to '%s'\n", filename);
}
