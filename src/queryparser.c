/* SQL Parser */
#include "queryparser.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
    if (length < 2) {
        char* unquoted = malloc(sizeof(*string)*(length+1));
        memcpy(unquoted, string, sizeof(*string)*length);
        unquoted[length] = '\0';
        return unquoted;
    }

    if (string[0] == QUERY_QUOTING_CHAR && string[length-1] == QUERY_QUOTING_CHAR) {
        size_t unquoted_length = length - 2;
        char* unquoted = malloc(sizeof(*string)*(unquoted_length+1));
        memcpy(unquoted, string + 1, sizeof(*string)*unquoted_length);
        unquoted[unquoted_length] = '\0';
        return unquoted;
    } else {
        char* unquoted = malloc(sizeof(*string)*(length+1));
        memcpy(unquoted, string, sizeof(*string)*length);
        unquoted[length] = '\0';
        return unquoted;
    }
}

QueryObject* parseQuery(char* query_string) {
    QueryObject* list = createQueryList();
    
    char* unquoted_token = NULL;
    char* cursor = query_string;
    char* token_start = cursor;

    while (1) {
        if (*cursor == QUERY_DELIMITER_CHAR || *cursor == '\0') {
            size_t token_length = cursor - token_start;

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
                    free(token);
                    break;
                case QUERY_FLOATING:
                    item = createQueryFloating(atof(token));
                    free(token);
                    break;
                case QUERY_STRING:
                    unquoted_token = unquoteString(token, token_length);
                    item = createQueryString(unquoted_token, strlen(unquoted_token));
                    free(unquoted_token);
                    free(token);
                    break;
                case QUERY_SYMBOL:
                    item = createQuerySymbol(token, token_length);
                    free(token);
                    break;
                default:
                    free(token);
                    perror("Unsupported token type in query parsing");
                    exit(1);
            }

            pushQueryItem(list, item);
            
            if (*cursor == '\0') break;
            
            token_start = cursor;
        }

        cursor++;
    }    
    
    return list;
}