#include <stdio.h>
#include "utils.h"
#include "csv.h"
void printCsvLine(CsvLine* csv_line, char delimiter, size_t col_size) {
  for (size_t j = 0; j < csv_line->count; j++) {
    CsvEntry* current_entry = csv_line->entries[j];

    switch (current_entry->type) {
      case INTEGER:
        printf("%-*d", (int) col_size, current_entry->integer);
        break;
      case FLOATING:
        printf("%-*f", (int) col_size, current_entry->floating);
        break;
      case STRING:
        printf("%-*.*s", (int) col_size, (int) col_size, current_entry->string.ptr);
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