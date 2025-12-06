/* SQL Parser */
#include "queryparser.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "csv.h"
#include "utils.h"
#include "csvparser.h"
#include "functions.h"

QueryObject* createQueryObject(enum QueryObjectType type) {
    QueryObject* object = malloc(sizeof(QueryObject));

    if (object == NULL) {
        perror("Unable to create QueryObject");
        exit(1);
    }

    object->type = type;
    object->refcount = 1;
    return object;
}

QueryObject* createQueryString(char *string, size_t length) {
    QueryObject* object = createQueryObject(QUERY_STRING);
    object->string.length = length;
    object->string.ptr = malloc(sizeof(*string)*length);

    if (object->string.ptr == NULL) {
        perror("Unable to allocate string for the QueryObject");
        exit(1);
    }

    memcpy(object->string.ptr, string, sizeof(*string)*length);

    return object;
}

QueryObject* createQueryInteger(int integer) {
    QueryObject* object = createQueryObject(QUERY_INTEGER);
    object->number = integer;
    return object;  
}

QueryObject* createQueryFloating(float floating) {
    QueryObject* object = createQueryObject(QUERY_FLOATING);
    object->number = floating;
    return object;
}

QueryObject* createQueryList() {
    QueryObject* object = createQueryObject(QUERY_LIST);
    object->list.count = 0;
    object->list.items = NULL;
    return object;
}

QueryObject* createQuerySymbol(char *symbol, size_t length) {
    QueryObject* object = createQueryObject(QUERY_SYMBOL);
    object->string.length = length;
    object->string.ptr = malloc(sizeof(*symbol)*length);

    if (object->string.ptr == NULL) {
        perror("Unable to allocate string for the QueryObject symbol");
        exit(1);
    }

    memcpy(object->string.ptr, symbol, sizeof(*symbol)*length);

    return object;
}

void pushQueryItem(QueryObject* list_object, QueryObject* item) {
    if (list_object->type != QUERY_LIST) {
        perror("Attempting to push item to non-list QueryObject");
        exit(1);
    }

    list_object->list.count++;
    list_object->list.items = realloc(list_object->list.items, sizeof(QueryObject*)*(list_object->list.count));

    if (list_object->list.items == NULL) {
        perror("Unable to realloc items for QueryObject list");
        exit(1);
    }

    list_object->list.items[list_object->list.count-1] = item;
}

void freeQueryObject(QueryObject* object) {
    if (object->type == QUERY_STRING || object->type == QUERY_SYMBOL) {
        free(object->string.ptr);
    }
    else if (object->type == QUERY_LIST) {
        for (size_t i = 0; i < object->list.count; i++) {
            freeQueryObject(object->list.items[i]);
        }
        free(object->list.items);
    }
    free(object);
}

void retainQueryObject(QueryObject* object) {
    object->refcount++;
}

void releaseQueryObject(QueryObject* object) {
    if (object->refcount == 0) {
        perror("Unable to release unreferenced QueryObject");
        exit(1);
    }

    if (--object->refcount == 0) {
        freeQueryObject(object);
    }
}

enum QueryObjectType inferQueryObjectType(char* token) {
    if (token == NULL || *token == '\0') {
        return QUERY_STRING;
    }

    char* cursor = token;
    int has_digits = 0;
    int has_dot = 0;
    int has_quote = 0;

    while (isspace(*cursor)) {
        cursor++;
    }
    
    if (*cursor == '-' || *cursor == '+') {
        cursor++;
    }

    if (*cursor == '\0') {
        return QUERY_STRING;
    }

    while (*cursor != '\0') {
        if (isdigit(*cursor)) {
            has_digits = 1;
            cursor++;
        }
        else if (*cursor == '.') {
            has_dot = 1;
            cursor++;
        } 
        else if (isspace(*cursor)) {
            cursor++;

            while (isspace(*cursor)) {
                cursor++;
            }
            
            if (*cursor != '\0') {
                return has_quote ? QUERY_STRING : QUERY_SYMBOL;
            }
            break;
        }  
        else if (*cursor == QUERY_QUOTING_CHAR) {
            has_quote = 1;
        }
        
        cursor++;
    }

    if (!has_digits) {
        return has_quote ? QUERY_STRING : QUERY_SYMBOL;
    }

    return has_dot ? QUERY_FLOATING : QUERY_INTEGER;
}

char* unquoteString(char* string, size_t length) {
    char* cursor = string;

    unsigned char quoting_char_found = 0;
    for(size_t i = 0; i < length; i++) {
        if (*cursor == QUERY_QUOTING_CHAR) {
            quoting_char_found = 1;
            break;
        }
        cursor++;
    }

    if (length < 2 || !quoting_char_found) {
        return string;
    }

    char* unquoted = malloc(sizeof(*string)*(length-2));
    memcpy(unquoted, string + 1, sizeof(*string)*(length-2));
    unquoted[length-2] = '\0';
    return unquoted;
}

QueryObject* parseQuery(char* query_string) {
    QueryObject* list = createQueryList();
    
    char* unquoted_token = NULL;
    char* cursor = query_string;
    char* token_start = cursor;

    while (1) {
        if (*cursor == QUERY_DELIMITER_CHAR || *cursor == '\0') {
            size_t token_length = cursor - token_start;

            while (token_length > 0 && isspace(*token_start)) {
                token_start++;
                token_length--;
            }

            if (token_length  == 0) {
                cursor++;
                token_start = cursor;
                continue;
            }

            char* token = malloc(sizeof(*token)*(token_length+1));
            memcpy(token, token_start, sizeof(*token)*token_length);
            token[token_length] = '\0';
            enum QueryObjectType type = inferQueryObjectType(token);

            QueryObject* item = NULL;

            switch (type) {
                case QUERY_INTEGER:
                    item = createQueryInteger(atoi(token));
                    break;
                case QUERY_FLOATING:
                    item = createQueryFloating(atof(token));
                    break;
                case QUERY_STRING:
                    unquoted_token = unquoteString(token, token_length);
                    item = createQueryString(unquoted_token, strlen(unquoted_token));
                    free(unquoted_token);
                    break;
                case QUERY_SYMBOL:
                    item = createQuerySymbol(token, token_length);
                    break;
                default:
                    free(token);
                    perror("Unsupported token type in query parsing");
                    exit(1);
            }

            pushQueryItem(list, item);
            
            if (*cursor == '\0') break;
            
            token_start = cursor;
            free(token);
        }

        cursor++;
    }    
    
    return list;
}

QueryFunctionRegistry* createFunctionRegistry(void) {
    QueryFunctionRegistry* registry = malloc(sizeof(QueryFunctionRegistry));
    if (registry == NULL) {
        perror("Unable to create QueryFunctionRegistry");
        exit(1);
    }
    registry->functions = NULL;
    registry->count = 0;
    return registry;
}

QueryFunction* createQueryFunction(char* name, void (*function_ptr)(QueryObject* context)) {
    QueryFunction* function = malloc(sizeof(QueryFunction));
    if (function == NULL) {
        perror("Unable to create QueryFunction");
        exit(1);
    }
    function->refcount = 1;
    function->name = malloc(sizeof(*name)*(strlen(name)+1));
    if (function->name == NULL) {
        perror("Unable to allocate name for QueryFunction");
        exit(1);
    }
    strcpy(function->name, name);
    function->function_ptr = function_ptr;
    return function;
}

void freeQueryFunction(QueryFunction* function) {
    if (function == NULL) return;
    free(function->name);
    free(function);
}

void freeFunctionRegistry(QueryFunctionRegistry* registry) {
    if (registry == NULL) return;
    for (size_t i = 0; i < registry->count; i++) {
        freeQueryFunction(registry->functions[i]);
    }
    free(registry->functions);
    free(registry);
}

void registerQueryFunction(QueryFunctionRegistry* registry, QueryFunction* function) {
    registry->count++;
    registry->functions = realloc(registry->functions, sizeof(QueryFunction*)*(registry->count));
    if (registry->functions == NULL) {
        perror("Unable to realloc functions in QueryFunctionRegistry");
        exit(1);
    }
    registry->functions[registry->count-1] = function;
}

QueryFunction* getQueryFunction(QueryFunctionRegistry* registry, char* name) {
    if (registry == NULL || registry->functions == NULL) return NULL;

    for (size_t i = 0; i < registry->count; i++) {
        if (strcmp(registry->functions[i]->name, name) == 0) {
            return registry->functions[i];
        }
    }
    return NULL;
}

QueryObject* popQueryItem(QueryObject* list_object) {
    if (list_object->type != QUERY_LIST || list_object->list.count == 0) {
        perror("Attempting to pop item from empty or non-list QueryObject");
        exit(1);
    }

    QueryObject* item = list_object->list.items[list_object->list.count - 1];
    list_object->list.count--;
    return item;
}

void executeQuery(QueryObject* query, CsvFile* csv_file, size_t data_line_index, QueryObject* context) {
    QueryObject* colums = createQueryList();
    QueryFunctionRegistry* function_registry = createFunctionRegistry();
    initQueryFunctionRegistry(function_registry);

    CsvLine* header = getHeaderLine(csv_file);
    for(size_t i = 0; i < header->count; i++) {
        CsvEntry* entry = header->entries[i];
        QueryObject* column = createQueryString(entry->string.ptr, entry->string.length);
        pushQueryItem(colums, column);
    }

    CsvLine* data_line = getDataLine(csv_file, data_line_index);

    for(size_t i = 0; i < query->list.count; i++) {
        QueryObject* item = query->list.items[i];

        if (item->type == QUERY_INTEGER) {
            QueryObject* integer_obj = createQueryInteger(item->number);
            pushQueryItem(context, integer_obj);
        }
        else if (item->type == QUERY_FLOATING) {
            QueryObject* floating_obj = createQueryFloating(item->number);
            pushQueryItem(context, floating_obj);
        }
        else if (item->type == QUERY_STRING) {
            QueryObject* string_obj = createQueryString(item->string.ptr, item->string.length);
            pushQueryItem(context, string_obj);
        }
        else if (item->type == QUERY_SYMBOL) {
            if (checkSymbol(colums, item->string.ptr)) {
                long col_index = label2index(csv_file, item->string.ptr);
                CsvEntry* data_entry = data_line->entries[col_index];
                
                if (data_entry->type == CSV_INTEGER) {
                    QueryObject* integer_obj = createQueryInteger(data_entry->number);
                    pushQueryItem(context, integer_obj);
                }
                else if (data_entry->type == CSV_STRING) {
                    QueryObject* string_obj = createQueryString(data_entry->string.ptr, data_entry->string.length);
                    pushQueryItem(context, string_obj);
                }
                else {
                    fprintf(stderr, "Unsupported CsvEntry type in query execution\n");
                    exit(1);
                }
            }
            else if(checkFunctionSymbol(function_registry, item->string.ptr)) {
                QueryFunction* function = getQueryFunction(function_registry, item->string.ptr);

                function->function_ptr(context);
            }
            else {
                fprintf(stderr, "Unknown symbol %s found in query execution\n", item->string.ptr);
                exit(1);
            }
        }
        else {
            fprintf(stderr, "Unsupported QueryObject type in query execution\n");
            exit(1);
        }

        /*printf("==========CONTEXT==========\n");
        printQueryObject(context);
        printf("===========================\n");*/
    }
}