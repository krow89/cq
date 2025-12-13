#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include "tokenizer.h"
#include "parser.h"
#include "evaluator.h"
#include "csv_reader.h"
#include "utils.h"


int main(int argc, char* argv[]) {
    char* query = NULL;
    char* query_file = NULL;
    char* output_file = NULL;
    bool print_count = false;
    bool print_table = false;
    bool vertical_output = false;
    bool query_allocated = false;  // track if we need to free query
    char input_separator = ',';
    char output_delimiter = ',';
    
    // parse args
    int opt;
    while ((opt = getopt(argc, argv, "hq:f:o:cps:d:v")) != -1) {
        switch (opt) {
            case 'h':
                print_help(argv[0]);
                return 0;
            case 'q':
                query = optarg;
                break;
            case 'f':
                query_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'c':
                print_count = true;
                break;
            case 'p':
                print_table = true;
                break;
            case 's':
                input_separator = optarg[0];
                break;
            case 'd':
                output_delimiter = optarg[0];
                break;
            case 'v':
                vertical_output = true;
                print_table = true;  // implicit
                break;
            default:
                print_help(argv[0]);
                return 1;
        }
    }
    
    // determine query source (priority: -f, -q, stdin)
    if (query_file) {
        // read from file
        query = read_query_from_file(query_file);
        if (!query) {
            return 1;
        }
        query_allocated = true;
    } else if (query) {
        // check if query is "-" which means read from stdin
        if (strcmp(query, "-") == 0) {
            query = read_query_from_stdin();
            if (!query) {
                return 1;
            }
            query_allocated = true;
        }
        // else use query as-is from command line
    } else {
        // no -q or -f specified
        fprintf(stderr, "Error: Query is required (use -q or -f)\n\n");
        print_help(argv[0]);
        return 1;
    }
    
    // global CSV configuration
    global_csv_config.delimiter = input_separator;
    global_csv_config.quote = '"';
    global_csv_config.has_header = true;
    
    // parse SQL query
    ASTNode* ast = parse(query);
    if (!ast) {
        fprintf(stderr, "Error: Parsing failed\n");
        return 1;
    }
    
    // evaluate query
    ResultSet* result = evaluate_query(ast);
    if (!result) {
        fprintf(stderr, "Error: Query evaluation failed\n");
        releaseNode(ast);
        return 1;
    }
    
    // output results based on flags
    if (print_count) {
        printf("Records: %d\n", result->row_count);
        printf("Columns: %d\n", result->column_count);
    }
    
    if (print_table) {
        if (vertical_output) {
            csv_print_table_vertical(result, result->row_count);
        } else {
            csv_print_table(result, result->row_count);
        }
    }
    
    if (output_file) {
        write_csv_file(output_file, result, output_delimiter);
    }
    
    // if no output options specified, default to count
    if (!print_count && !print_table && !output_file) {
        printf("Count: %d\n", result->row_count);
    }
    
    // cleanup
    csv_free(result);
    releaseNode(ast);
    if (query_allocated) {
        free(query);
    }
    
    return 0;
}
