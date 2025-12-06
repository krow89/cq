#include "csvparser.h"

enum CsvDataEntry inferDataType(char* string) {
  if (string == NULL || *string == '\0') {
    return CSV_STRING;
  }

  char* cursor = string;
  int has_dot = 0;
  int has_digits = 0;

  while (isspace(*cursor)) {
    cursor++;
  }

  if (*cursor == '+' || *cursor == '-') {
    cursor++;
  }

  if (*cursor == '\0') {
    return CSV_STRING;
  }

  while (*cursor != '\0') {
    if (isdigit(*cursor)) {
      has_digits = 1;
      cursor++;
    }
    else if (*cursor == '.' && !has_dot) {
      has_dot = 1;
      cursor++;
    }
    else if (isspace(*cursor)) {
      cursor++;

      while (isspace(*cursor)) {
        cursor++;
      }
      
      if (*cursor != '\0') {
        return CSV_STRING;
      }
      break;
    } else {
      return CSV_STRING;
    }
  }
  if (!has_digits) {
    return CSV_STRING;
  }

  return has_dot ? CSV_FLOATING : CSV_INTEGER;
}

CsvLine* parseLine(CsvEntry* line, size_t line_number) {
  CsvLine* csv_line = createCsvLine();
  char* cursor = line->string.ptr;
  size_t char_count = 0;
  size_t field_offset = 0;

  while (*cursor != '\0') {
    while(*cursor != CSV_DELIMITER_CHAR && *cursor != '\0') {
      cursor++;
      char_count++;
    }

    size_t field_length = char_count-field_offset;

    char* field = malloc(sizeof(*field)*(field_length+1));
    memcpy(field, line->string.ptr+field_offset, field_length);
    field[field_length] = '\0';

    enum CsvDataEntry type = inferDataType(field);
    CsvEntry* entry = NULL;

    if (line_number == 0) {
      type = CSV_STRING;
    }

    switch (type) {
      case CSV_INTEGER:
        entry = createCsvInteger(atoi(field));
        break;
      case CSV_FLOATING:
        entry = createCsvFloating(atof(field));
        break;
      case CSV_STRING:
        entry = createCsvString(field, field_length);
        break;
    }

    pushEntry(csv_line, entry);

    free(field);
    field_offset = char_count+1;
    char_count++;
    cursor++;
  }

  return csv_line;
}

CsvFile* parseFile(char* file_buffer) {
  CsvFile* csv_file = createCsvFile();
  size_t indentation_level = 0;
  size_t line_number = 0;
  char* cursor = file_buffer;

  while(*cursor != '\0') {
    size_t line_length = 0;
    while (*cursor != '\n' && *cursor != '\0') {
      line_length++;
      cursor++;
    }

    char* line_string = malloc(line_length+1);
    memcpy(line_string, cursor - line_length, line_length);
    line_string[line_length] = '\0';
    CsvEntry* line = createCsvString(line_string, line_length);
    free(line_string);

    char* line_ptr = cursor-line_length; 
    size_t current_indentation_level = 0;

    while(isspace(*line_ptr)) {
      current_indentation_level++;
      line_ptr++;
    }

    if (indentation_level != current_indentation_level) {
      fprintf(stderr, "Single indentation only supported for now\n");
      exit(1);
    }

    CsvLine* csv_line = parseLine(line, line_number);
    pushLine(csv_file, csv_line);

    freeCsvEntry(line);
    indentation_level = current_indentation_level;
    cursor++;
    line_number++;
  }

  return csv_file;
}


long label2index(CsvFile* file, char* label) {
  if (file->count == 0) {
    perror("File has no lines");
    exit(1);
  }

  CsvLine* header = file->lines[0];

  for (size_t i = 0; i < header->count; i++) {
    CsvEntry* entry = header->entries[i];;

    if (strcmp(entry->string.ptr, label) == 0) {
      return i;
    }
  }

  return 0;
}

char* index2label(CsvFile* file, size_t index) {
  if (file->count == 0) {
    perror("File has no lines");
    exit(1);
  }

  CsvLine* header = file->lines[0];

  if (index >= header->count) {
    perror("Index out of bounds");
    exit(1);
  }

  CsvEntry* entry = header->entries[index];

  return entry->string.ptr;
}
