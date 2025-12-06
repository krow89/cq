#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define DELIMITER_CHAR ','
#define QUOTING_CHAR '"' 

enum CsvDataEntry {
  INTEGER,
  FLOATING,
  STRING,
};

typedef struct CsvEntry {
  size_t refcount;
  enum CsvDataEntry type;
  union {
    int integer;
    float floating;
    struct {
      char* ptr;
      size_t length;
    } string;
  };
} CsvEntry;

typedef struct CsvLine {
  size_t count;
  CsvEntry** entries;
} CsvLine;

typedef struct CsvFile {
  size_t count;
  CsvLine** lines;
} CsvFile;

CsvLine* createCsvLine() {
  CsvLine* line = malloc(sizeof(*line));
  line->count = 0;
  line->entries = NULL;
  return line;
}

CsvFile* createCsvFile() {
  CsvFile* file = malloc(sizeof(*file));
  file->count = 0;
  file->lines = NULL;
  return file;
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

CsvLine* getDataLine(CsvFile* file, size_t line_number) {
  if (file->count < 2) {
    perror("File has no data lines");
    exit(1);
  }

  if (line_number + 1 >= file->count) {
    perror("Line number out of bounds");
    exit(1);
  }

  return file->lines[line_number + 1];
}

CsvLine* getHeaderLine(CsvFile* file) {
  if (file->count == 0) {
    perror("File has no lines");
    exit(1);
  }

  return file->lines[0];
}

void pushEntry(CsvLine *line, CsvEntry* entry) {
  line->count++;
  line->entries = realloc(line->entries, sizeof(*entry)*(line->count));
  if (line->entries == NULL) {
    perror("Unable to realloc entries");
    exit(1);
  }
  line->entries[line->count-1] = entry;
}

void pushLine(CsvFile *file, CsvLine* line) {
  file->count++;
  file->lines = realloc(file->lines, sizeof(*line)*(file->count));
  if (file->lines == NULL) {
    perror("Unable to realloc lines");
    exit(1);
  }
  file->lines[file->count-1] = line;
}

CsvEntry* createCsvEntry(enum CsvDataEntry type) {
  CsvEntry* entry = malloc(sizeof(CsvEntry));
  if (entry == NULL) {
    perror("Unable to create entry");
    exit(1);
  }
  entry->type = type;
  entry->refcount = 1;
  return entry;
}

CsvEntry* createCsvString(char *string, size_t length) {
  CsvEntry* entry = createCsvEntry(STRING);
  entry->string.length = length;
  entry->string.ptr = malloc(sizeof(*string)*length);
  if (entry->string.ptr == NULL) {
    perror("Unable to allocate string for the entry");
    exit(1);
  }
  memcpy(entry->string.ptr, string, sizeof(*string)*length);
  return entry;
}

CsvEntry* createCsvInteger(int integer) {
  CsvEntry* entry = createCsvEntry(INTEGER);
  entry->integer = integer;
  return entry;
}

CsvEntry* createCsvFloating(float floating) {
  CsvEntry* entry = createCsvEntry(FLOATING);
  entry->floating = floating;
  return entry;
}

void retain(CsvEntry* object) {
  object->refcount++;
}

void release(CsvEntry* object) {
  if(object->refcount == 0) {
    perror("Unable to release unreferenced entry");
    exit(1);
  }

  object->refcount--;

  if (object->refcount == 0) {
    free(object);
  }
}

enum CsvDataEntry inferDataType(char* string) {
  if (string == NULL || *string == '\0') {
    return STRING;
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
    return STRING;
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
        return STRING;
      }
      break;
    } else {
      return STRING;
    }
  }
  if (!has_digits) {
    return STRING;
  }

  return has_dot ? FLOATING : INTEGER;
}

CsvLine* parseLine(CsvEntry* line, size_t line_number) {
  CsvLine* csv_line = createCsvLine();
  char* cursor = line->string.ptr;
  size_t char_count = 0;
  size_t field_offset = 0;

  while (*cursor != '\0') {
    while(*cursor != DELIMITER_CHAR && *cursor != '\0') {
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
      type = STRING;
    }

    switch (type) {
      case INTEGER:
        entry = createCsvInteger(atoi(field));
        break;
      case FLOATING:
        entry = createCsvFloating(atof(field));
        break;
      case STRING:
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

CsvFile* parse(char* file_buffer) {
  CsvFile* csv_file = createCsvFile();
  size_t indentation_level = 0;
  size_t line_number = 0;
  char* cursor = file_buffer;

  while(*cursor != '\0') {
    size_t line_length = 0;
    while (*cursor != '\n') {
      line_length++;
      cursor++;
    }

    CsvEntry* line = createCsvString("", line_length+1);
    memcpy(line->string.ptr, cursor-line_length, line_length+1);
    line->string.ptr[line_length] = '\0';

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

    free(line);
    indentation_level = current_indentation_level;
    cursor++;
    line_number++;
  }

  return csv_file;
}

void printCsvLine(CsvLine* csv_line, char delimiter, size_t col_size) {
  for (size_t j = 0; j < csv_line->count; j++) {
    CsvEntry* current_entry = csv_line->entries[j];

    switch (current_entry->type) {
      case INTEGER:
        printf("%-*d", col_size, current_entry->integer);
        break;
      case FLOATING:
        printf("%-*f", col_size, current_entry->floating);
        break;
      case STRING:
        printf("%-*.*s", col_size, col_size, current_entry->string.ptr);
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


int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <file_name>\n", argv[0]);
    exit(1);
  }

  FILE *fp = fopen(argv[1], "r");

  if (fp == NULL) {
    perror("Error opening CSV file");
    exit(1);
  }
  
  fseek(fp, 0, SEEK_END);
  size_t file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char* file_buffer = malloc(sizeof(*file_buffer)*(file_size+1));
  size_t bytes_read = fread(file_buffer, 1, file_size, fp);
  
  if (bytes_read == 0 | bytes_read != file_size) {
    fprintf(stderr, "Error reading the file, read %zu bytes, expected %zu\n", bytes_read, file_size);
    exit(1);
  }

  file_buffer[file_size] = '\0';

  fclose(fp);

  CsvFile* csv_file = parse(file_buffer);
  
  printCsvFile(csv_file);

  free(file_buffer);
  return 0;
}
