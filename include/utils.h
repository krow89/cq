#ifndef UTILS_H
#define UTILS_H

#include "csv.h"
#include "queryparser.h"

void printCsvLine(CsvLine* csv_line, char delimiter, size_t col_size);
void printCsvHeaderLine(CsvLine* csv_line);
void printCsvDataLine(CsvLine* csv_line);
void printCsvFile(CsvFile* csv_file);
void printQueryObject(QueryObject* object);

#endif