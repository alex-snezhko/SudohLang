#pragma once
#include "variable.h"

Variable f_input();
Variable f_print(Variable str);
Variable f_printLine(Variable str);
Variable f_length(Variable var);
Variable f_string(Variable var);
Variable f_integer(Variable var);
Variable f_number(Variable str);
Variable f_ascii(Variable num);
Variable f_random(Variable range);
Variable f_remove(Variable var, Variable index);
Variable f_removeLast(Variable list);
Variable f_append(Variable list, Variable value);
Variable f_insert(Variable list, Variable index, Variable value);
Variable f_range(Variable str, Variable begin, Variable end);
Variable f_type(Variable var);

Variable f_pow(Variable num, Variable pow);
Variable f_cos(Variable num);
Variable f_sin(Variable num);
Variable f_tan(Variable num);
Variable f_acos(Variable num);
Variable f_asin(Variable num);
Variable f_atan(Variable num);
Variable f_atan2(Variable num1, Variable num2);
Variable f_log(Variable num, Variable base);
