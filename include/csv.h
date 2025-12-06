#ifndef CSV_H
#define CSV_H

#include <stdlib.h>

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

CsvLine* createCsvLine(void);
CsvFile* createCsvFile(void);
CsvEntry* createCsvEntry(enum CsvDataEntry type);
CsvEntry* createCsvString(char *string, size_t length);
CsvEntry* createCsvInteger(int integer);
CsvEntry* createCsvFloating(float floating);
CsvLine* getDataLine(CsvFile* file, size_t line_number);
CsvLine* getHeaderLine(CsvFile* file);
void pushEntry(CsvLine *line, CsvEntry* entry);
void pushLine(CsvFile *file, CsvLine* line);
void freeCsvEntry(CsvEntry* entry);
void freeCsvLine(CsvLine* line);
void freeCsvFile(CsvFile* file);
void retain(CsvEntry* object);
void release(CsvEntry* object);

#endif