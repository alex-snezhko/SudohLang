#ifndef SUDOH_H
#define SUDOH_H

#include "variable.h"

#define LIST (std::shared_ptr<Variable::List>)new Variable::List
#define OBJECT (std::shared_ptr<Variable::Object>)new Variable::Object

const Variable null = Variable();

typedef Variable var;

Variable p_input();
Variable p_print(Variable str);
Variable p_printLine(Variable str);
Variable p_length(Variable var);
Variable p_string(Variable var);
Variable p_integer(Variable var);
Variable p_number(Variable str);
Variable p_ascii(Variable num);
Variable p_random(Variable range);
Variable p_remove(Variable var, Variable index);
Variable p_removeLast(Variable list);
Variable p_append(Variable list, Variable value);
Variable p_insert(Variable list, Variable index, Variable value);
Variable p_range(Variable str, Variable begin, Variable end);
Variable p_type(Variable var);

Variable p_pow(Variable num, Variable pow);
Variable p_cos(Variable num);
Variable p_sin(Variable num);
Variable p_tan(Variable num);
Variable p_acos(Variable num);
Variable p_asin(Variable num);
Variable p_atan(Variable num);
Variable p_atan2(Variable num1, Variable num2);
Variable p_log(Variable num, Variable base);

#endif
