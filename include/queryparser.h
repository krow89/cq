/* SQL Parser */
#ifndef QUERYPARSER_H
#define QUERYPARSER_H

#include <stdlib.h>

typedef struct Column {
    char* name;
} Column;

typedef struct SelectItem {
    Column* column;
    char* alias;
} SelectItem;

typedef struct SelectList {
    SelectItem** items;
    size_t count;
} SelectList;

/*typedef struct SelectExpression {
    // void (*callback)(SelectList*);
} SelectExpression;

typedef struct WhereExpression {
    WhereClauseList* clauses;
} WhereExpression;*/

#endif
