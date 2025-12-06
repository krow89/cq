#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "csv.h"
#include "queryparser.h"

void printCsvLine(CsvLine* csv_line, char delimiter, size_t col_size) {
  for (size_t j = 0; j < csv_line->count; j++) {
    CsvEntry* current_entry = csv_line->entries[j];

    switch (current_entry->type) {
      case CSV_INTEGER:
        printf("%-*d", (int) col_size, (int) current_entry->number);
        break;
      case CSV_FLOATING:
        printf("%-*f", (int) col_size, (float) current_entry->number);
        break;
      case CSV_STRING:
        printf("%-*.*s", (int) col_size, (int) (col_size < current_entry->string.length ? col_size : current_entry->string.length), current_entry->string.ptr);
    }

    if (j < csv_line->count - 1) {
      printf("%c", delimiter);
    }
  }
}

void printCsvHeaderLine(CsvLine* csv_line) {
  char delimiter = '|';
  printf("%c", delimiter);
  printCsvLine(csv_line, delimiter, 10);
  printf("%c\n", delimiter);

  size_t char_count = csv_line->count + csv_line->count*10 + 1;

  for(size_t i = 0; i < char_count; i++) {
    printf("=");
  }

  printf("\n");
}

void printCsvDataLine(CsvLine* csv_line) {
  char delimiter = ' ';
  printCsvLine(csv_line, delimiter, 10);
  printf("%c\n", delimiter);
}

void printCsvFile(CsvFile* csv_file) {
  if (csv_file == NULL) {
    printf("CSV file is NULL\n");
    return;
  }

  if (csv_file->count == 0) {
    printf("CSV file has no lines\n");
    return;
  }

  CsvLine* header_line = csv_file->lines[0];
  printCsvHeaderLine(header_line);

  for (size_t i = 0; i < csv_file->count - 1; i++) {
    CsvLine* current_line = getDataLine(csv_file, i);

    printCsvDataLine(current_line);
  }
}

void printQueryObject(QueryObject* object) {
  if (object == NULL) {
    printf("QueryObject is NULL\n");
    return;
  }

  switch (object->type) {
    case QUERY_STRING:
      printf("String: %.*s\n", (int)object->string.length, object->string.ptr);
      break;
    case QUERY_INTEGER:
      printf("Integer: %d\n", (int) object->number);
      break;
    case QUERY_FLOATING:
      printf("Floating: %f\n", (float) object->number);
      break;
    case QUERY_SYMBOL:
      printf("Symbol: %.*s\n", (int)object->string.length, object->string.ptr);
      break;
    case QUERY_LIST:
      printf("List with %zu items:\n", object->list.count);
      for (size_t i = 0; i < object->list.count; i++) {
        printQueryObject(object->list.items[i]);
      }
      break;
    default:
      printf("Unknown QueryObject type\n");
  }
}

unsigned char checkSymbol(QueryObject* symbols, char* symbol) {
  for (size_t i = 0; i < symbols->list.count; i++) {
    QueryObject* item = symbols->list.items[i];
    if (strncmp(item->string.ptr, symbol, item->string.length) == 0) {
      return 1;
    }
  }
  return 0;
}

unsigned char checkFunctionSymbol(QueryFunctionRegistry* functions, char* symbol) {
  QueryFunction* function = getQueryFunction(functions, symbol);
  return function != NULL;
}