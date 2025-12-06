#ifndef CSVPARSER_H
#define CSVPARSER_H

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "csv.h"

enum CsvDataEntry inferDataType(char* string);
CsvLine* parseLine(CsvEntry* line, size_t line_number);
CsvFile* parseFile(char* file_buffer);
long label2index(CsvFile* file, char* label);
char* index2label(CsvFile* file, size_t index);

#endif