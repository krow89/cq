/* Stack Based Language Parser */
#ifndef QUERYPARSER_H
#define QUERYPARSER_H

#include <stdlib.h>
#include "csv.h"

#define QUERY_DELIMITER_CHAR ' '
#define QUERY_QUOTING_CHAR '"' 

enum QueryObjectType {
    QUERY_STRING,
    QUERY_INTEGER,
    QUERY_FLOATING,
    QUERY_LIST,
    QUERY_SYMBOL,
};

typedef struct QueryObject {
    enum QueryObjectType type;
    size_t refcount;
    union {
        int number;
        struct {
            char* ptr;
            size_t length;
        } string;
        struct {
            struct QueryObject** items;
            size_t count;
        } list;
    };
} QueryObject;

typedef struct QueryFunction {
    size_t refcount;
    char* name;
    void (*function_ptr)(QueryObject* context);
} QueryFunction;

typedef struct QueryFunctionRegistry {
    QueryFunction** functions;
    size_t count;
} QueryFunctionRegistry;

QueryObject* createQueryObject(enum QueryObjectType type);
QueryObject* createQueryString(char *string, size_t length);
QueryObject* createQueryInteger(int integer);
QueryObject* createQueryFloating(float floating);
QueryObject* createQueryList();
QueryObject* createQuerySymbol(char *symbol, size_t length);
void pushQueryItem(QueryObject* list_object, QueryObject* item);
void freeQueryObject(QueryObject* object);
void retainQueryObject(QueryObject* object);
void releaseQueryObject(QueryObject* object);
enum QueryObjectType inferQueryObjectType(char* token);
QueryObject* parseQuery(char* query_string);
char* unquoteString(char* string, size_t length);
void executeQuery(QueryObject* query, CsvFile* csv_file, size_t data_line_index, QueryObject* context);
QueryFunctionRegistry* createFunctionRegistry(void);
QueryFunction* createQueryFunction(char* name, size_t arg_count, void (*function_ptr)(QueryObject* context));
void freeQueryFunction(QueryFunction* function);
void freeFunctionRegistry(QueryFunctionRegistry* registry);
void registerQueryFunction(QueryFunctionRegistry* registry, QueryFunction* function);
QueryFunction* getQueryFunction(QueryFunctionRegistry* registry, char* name);
QueryObject* popQueryItem(QueryObject* list_object);

#endif
