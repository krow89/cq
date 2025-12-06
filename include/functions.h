#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "queryparser.h"

void initQueryFunctionRegistry(QueryFunctionRegistry* registry);
void qf_eq(QueryObject* context);
void qf_gt(QueryObject* context);
void qf_and(QueryObject* context);

#endif