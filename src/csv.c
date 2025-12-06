#include "csv.h"

#include <string.h>
#include <stdio.h>

CsvLine* createCsvLine(void) {
  CsvLine* line = malloc(sizeof(*line));
  line->count = 0;
  line->entries = NULL;
  return line;
}

CsvFile* createCsvFile(void) {
  CsvFile* file = malloc(sizeof(*file));
  file->count = 0;
  file->lines = NULL;
  return file;
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

void freeCsvEntry(CsvEntry* entry) {
  if (entry->type == STRING) {
    free(entry->string.ptr);
  }
  free(entry);
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
    freeCsvEntry(object);
  }
}