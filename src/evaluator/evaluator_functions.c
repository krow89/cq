#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include "evaluator.h"
#include "evaluator/evaluator_functions.h"

/* helper function to transform string case */
static char* transform_string_case(const char* str, bool to_upper) {
    if (!str) return NULL;
    
    char* result = strdup(str);
    for (int i = 0; result[i]; i++) {
        result[i] = to_upper ? toupper(result[i]) : tolower(result[i]);
    }
    return result;
}

/* evaluate scalar functions, handles concat, lower, upper, length, substring, replace, coalesce, power, sqrt, ceil, floor, round, abs, exp, ln, mod */
Value evaluate_scalar_function(const char* func_name, Value* args, int arg_count) {
    Value result;
    result.type = VALUE_TYPE_NULL;
    
    if (arg_count < 1) return result;
    
    // CONCAT
    if (strcasecmp(func_name, "CONCAT") == 0) {
        char buffer[1024] = "";
        for (int i = 0; i < arg_count; i++) {
            if (args[i].type == VALUE_TYPE_STRING && args[i].string_value) {
                strcat(buffer, args[i].string_value);
            } else if (args[i].type == VALUE_TYPE_INTEGER) {
                char temp[64];
                snprintf(temp, sizeof(temp), "%lld", args[i].int_value);
                strcat(buffer, temp);
            } else if (args[i].type == VALUE_TYPE_DOUBLE) {
                char temp[64];
                snprintf(temp, sizeof(temp), "%.2f", args[i].double_value);
                strcat(buffer, temp);
            }
        }
        result.type = VALUE_TYPE_STRING;
        result.string_value = strdup(buffer);
        return result;
    }
    
    // LOWER
    if (strcasecmp(func_name, "LOWER") == 0) {
        if (args[0].type == VALUE_TYPE_STRING && args[0].string_value) {
            result.type = VALUE_TYPE_STRING;
            result.string_value = transform_string_case(args[0].string_value, false);
        }
        return result;
    }
    
    // UPPER
    if (strcasecmp(func_name, "UPPER") == 0) {
        if (args[0].type == VALUE_TYPE_STRING && args[0].string_value) {
            result.type = VALUE_TYPE_STRING;
            result.string_value = transform_string_case(args[0].string_value, true);
        }
        return result;
    }
    
    // LENGTH
    if (strcasecmp(func_name, "LENGTH") == 0) {
        if (args[0].type == VALUE_TYPE_STRING && args[0].string_value) {
            result.type = VALUE_TYPE_INTEGER;
            result.int_value = strlen(args[0].string_value);
        }
        return result;
    }
    
    // SUBSTRING(str, start, length)
    if (strcasecmp(func_name, "SUBSTRING") == 0 && arg_count >= 3) {
        if (args[0].type == VALUE_TYPE_STRING && args[0].string_value &&
            args[1].type == VALUE_TYPE_INTEGER && args[2].type == VALUE_TYPE_INTEGER) {
            
            int start = args[1].int_value - 1; // convert to 0-indexed
            int length = args[2].int_value;
            const char* str = args[0].string_value;
            int str_len = strlen(str);
            
            if (start < 0) start = 0;
            if (start >= str_len) {
                result.type = VALUE_TYPE_STRING;
                result.string_value = strdup("");
                return result;
            }
            
            if (start + length > str_len) {
                length = str_len - start;
            }
            
            char* substr = malloc(length + 1);
            strncpy(substr, str + start, length);
            substr[length] = '\0';
            
            result.type = VALUE_TYPE_STRING;
            result.string_value = substr;
        }
        return result;
    }
    
    // REPLACE(str, from, to)
    if (strcasecmp(func_name, "REPLACE") == 0 && arg_count >= 3) {
        if (args[0].type == VALUE_TYPE_STRING && args[0].string_value &&
            args[1].type == VALUE_TYPE_STRING && args[1].string_value &&
            args[2].type == VALUE_TYPE_STRING && args[2].string_value) {
            
            const char* str = args[0].string_value;
            const char* from = args[1].string_value;
            const char* to = args[2].string_value;
            
            int from_len = strlen(from);
            int to_len = strlen(to);
            
            if (from_len == 0) {
                result.type = VALUE_TYPE_STRING;
                result.string_value = strdup(str);
                return result;
            }
            
            // count occurrences
            int count = 0;
            const char* pos = str;
            while ((pos = strstr(pos, from)) != NULL) {
                count++;
                pos += from_len;
            }
            
            // allocate result buffer
            int result_len = strlen(str) + count * (to_len - from_len);
            char* new_str = malloc(result_len + 1);
            char* dest = new_str;
            
            pos = str;
            while (*pos) {
                if (strncmp(pos, from, from_len) == 0) {
                    strcpy(dest, to);
                    dest += to_len;
                    pos += from_len;
                } else {
                    *dest++ = *pos++;
                }
            }
            *dest = '\0';
            
            result.type = VALUE_TYPE_STRING;
            result.string_value = new_str;
        }
        return result;
    }
    
    // COALESCE
    if (strcasecmp(func_name, "COALESCE") == 0) {
        for (int i = 0; i < arg_count; i++) {
            if (args[i].type != VALUE_TYPE_NULL) {
                // deep copy the value to avoid freeing shared pointers
                result.type = args[i].type;
                if (args[i].type == VALUE_TYPE_STRING && args[i].string_value) {
                    result.string_value = strdup(args[i].string_value);
                } else {
                    result.int_value = args[i].int_value;
                }
                return result;
            }
        }
        return result;
    }
    
    // POWER(base, exponent)
    if (strcasecmp(func_name, "POWER") == 0 && arg_count >= 2) {
        double base = 0, exponent = 0;
        if (args[0].type == VALUE_TYPE_INTEGER) {
            base = (double)args[0].int_value;
        } else if (args[0].type == VALUE_TYPE_DOUBLE) {
            base = args[0].double_value;
        } else {
            return result;
        }
        
        if (args[1].type == VALUE_TYPE_INTEGER) {
            exponent = (double)args[1].int_value;
        } else if (args[1].type == VALUE_TYPE_DOUBLE) {
            exponent = args[1].double_value;
        } else {
            return result;
        }
        
        result.type = VALUE_TYPE_DOUBLE;
        result.double_value = pow(base, exponent);
        return result;
    }
    
    // SQRT(number)
    if (strcasecmp(func_name, "SQRT") == 0) {
        double val = 0;
        if (args[0].type == VALUE_TYPE_INTEGER) {
            val = (double)args[0].int_value;
        } else if (args[0].type == VALUE_TYPE_DOUBLE) {
            val = args[0].double_value;
        } else {
            return result;
        }
        
        if (val < 0) {
            return result; // null for negative numbers
        }
        
        result.type = VALUE_TYPE_DOUBLE;
        result.double_value = sqrt(val);
        return result;
    }
    
    // CEIL(number)
    if (strcasecmp(func_name, "CEIL") == 0 || strcasecmp(func_name, "CEILING") == 0) {
        double val = 0;
        if (args[0].type == VALUE_TYPE_INTEGER) {
            result.type = VALUE_TYPE_INTEGER;
            result.int_value = args[0].int_value;
            return result;
        } else if (args[0].type == VALUE_TYPE_DOUBLE) {
            val = args[0].double_value;
        } else {
            return result;
        }
        
        result.type = VALUE_TYPE_DOUBLE;
        result.double_value = ceil(val);
        return result;
    }
    
    // FLOOR(number)
    if (strcasecmp(func_name, "FLOOR") == 0) {
        double val = 0;
        if (args[0].type == VALUE_TYPE_INTEGER) {
            result.type = VALUE_TYPE_INTEGER;
            result.int_value = args[0].int_value;
            return result;
        } else if (args[0].type == VALUE_TYPE_DOUBLE) {
            val = args[0].double_value;
        } else {
            return result;
        }
        
        result.type = VALUE_TYPE_DOUBLE;
        result.double_value = floor(val);
        return result;
    }
    
    // ROUND(number, [decimals])
    if (strcasecmp(func_name, "ROUND") == 0) {
        double val = 0;
        int decimals = 0;
        
        if (args[0].type == VALUE_TYPE_INTEGER) {
            val = (double)args[0].int_value;
        } else if (args[0].type == VALUE_TYPE_DOUBLE) {
            val = args[0].double_value;
        } else {
            return result;
        }
        
        // optional second argument for decimal places
        if (arg_count >= 2) {
            if (args[1].type == VALUE_TYPE_INTEGER) {
                decimals = (int)args[1].int_value;
            } else if (args[1].type == VALUE_TYPE_DOUBLE) {
                decimals = (int)args[1].double_value;
            }
        }
        
        // round to specified decimal places
        double multiplier = pow(10.0, decimals);
        result.type = VALUE_TYPE_DOUBLE;
        result.double_value = round(val * multiplier) / multiplier;
        
        // if no decimals specified and result is whole number, return as integer
        if (decimals == 0 && result.double_value == floor(result.double_value)) {
            result.type = VALUE_TYPE_INTEGER;
            result.int_value = (long long)result.double_value;
        }
        
        return result;
    }
    
    // ABS(number)
    if (strcasecmp(func_name, "ABS") == 0) {
        if (args[0].type == VALUE_TYPE_INTEGER) {
            result.type = VALUE_TYPE_INTEGER;
            result.int_value = llabs(args[0].int_value);
            return result;
        } else if (args[0].type == VALUE_TYPE_DOUBLE) {
            result.type = VALUE_TYPE_DOUBLE;
            result.double_value = fabs(args[0].double_value);
            return result;
        }
        return result;
    }
    
    // EXP(number) - e^x
    if (strcasecmp(func_name, "EXP") == 0) {
        double val = 0;
        if (args[0].type == VALUE_TYPE_INTEGER) {
            val = (double)args[0].int_value;
        } else if (args[0].type == VALUE_TYPE_DOUBLE) {
            val = args[0].double_value;
        } else {
            return result;
        }
        
        result.type = VALUE_TYPE_DOUBLE;
        result.double_value = exp(val);
        return result;
    }
    
    // LN(number) - natural logarithm
    if (strcasecmp(func_name, "LN") == 0 || strcasecmp(func_name, "LOG") == 0) {
        double val = 0;
        if (args[0].type == VALUE_TYPE_INTEGER) {
            val = (double)args[0].int_value;
        } else if (args[0].type == VALUE_TYPE_DOUBLE) {
            val = args[0].double_value;
        } else {
            return result;
        }
        
        if (val <= 0) {
            return result; // null for non-positive numbers
        }
        
        result.type = VALUE_TYPE_DOUBLE;
        result.double_value = log(val);
        return result;
    }
    
    // MOD(dividend, divisor)
    if (strcasecmp(func_name, "MOD") == 0 && arg_count >= 2) {
        if (args[0].type == VALUE_TYPE_INTEGER && args[1].type == VALUE_TYPE_INTEGER) {
            if (args[1].int_value == 0) {
                return result; // null for division by zero
            }
            result.type = VALUE_TYPE_INTEGER;
            result.int_value = args[0].int_value % args[1].int_value;
            return result;
        } else {
            double dividend = 0, divisor = 0;
            if (args[0].type == VALUE_TYPE_INTEGER) {
                dividend = (double)args[0].int_value;
            } else if (args[0].type == VALUE_TYPE_DOUBLE) {
                dividend = args[0].double_value;
            } else {
                return result;
            }
            
            if (args[1].type == VALUE_TYPE_INTEGER) {
                divisor = (double)args[1].int_value;
            } else if (args[1].type == VALUE_TYPE_DOUBLE) {
                divisor = args[1].double_value;
            } else {
                return result;
            }
            
            if (divisor == 0) {
                return result; // null for division by zero
            }
            
            result.type = VALUE_TYPE_DOUBLE;
            result.double_value = fmod(dividend, divisor);
            return result;
        }
    }
    
    return result;
}
