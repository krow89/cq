#include "functions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void initQueryFunctionRegistry(QueryFunctionRegistry* registry) {
    registerQueryFunction(registry, createQueryFunction("EQ", 2, qf_eq));
    registerQueryFunction(registry, createQueryFunction("GT", 2, qf_gt));
    registerQueryFunction(registry, createQueryFunction("AND", 2, qf_and));
}

void qf_eq(QueryObject* context) {
    QueryObject* obj2 = popQueryItem(context);
    QueryObject* obj1 = popQueryItem(context);
    unsigned char is_equal = 0;

    if (obj1->type != obj2->type) {
        is_equal = 0;
    } else {
        switch (obj1->type) {
            case QUERY_INTEGER:
                is_equal = (obj1->number == obj2->number);
                break;
            case QUERY_FLOATING:
                is_equal = ((float)obj1->number == (float)obj2->number);
                break;
            case QUERY_STRING:
                is_equal = (obj1->string.length == obj2->string.length) &&
                           (strncmp(obj1->string.ptr, obj2->string.ptr, obj1->string.length) == 0);
                break;
            default:
                fprintf(stderr, "Unsupported QueryObject type for equality check\n");
                exit(1);
        }
    }

    pushQueryItem(context, createQueryInteger(is_equal));
}

void qf_gt(QueryObject* context) {
    QueryObject* obj2 = popQueryItem(context);
    QueryObject* obj1 = popQueryItem(context);
    unsigned char is_greater = 0;

    if (obj1->type != obj2->type) {
        fprintf(stderr, "Type mismatch in GT function\n");
        exit(1);
    } else {
        switch (obj1->type) {
            case QUERY_INTEGER:
                is_greater = (obj1->number > obj2->number);
                break;
            case QUERY_FLOATING:
                is_greater = ((float)obj1->number > (float)obj2->number);
                break;
            default:
                fprintf(stderr, "Unsupported QueryObject type for greater-than check\n");
                exit(1);
        }
    }

    pushQueryItem(context, createQueryInteger(is_greater));
}
    
void qf_and(QueryObject* context) {
    QueryObject* obj2 = popQueryItem(context);
    QueryObject* obj1 = popQueryItem(context);
    unsigned char result = 0;

    if (obj1->type != QUERY_INTEGER || obj2->type != QUERY_INTEGER) {
        fprintf(stderr, "AND function requires integer (boolean) arguments\n");
        exit(1);
    } else {
        result = (obj1->number && obj2->number);
    }

    pushQueryItem(context, createQueryInteger(result));
}