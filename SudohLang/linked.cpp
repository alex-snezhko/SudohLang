#include <iostream>
#include <string>
#include <vector>
#include <map>

using namespace std;

struct Variable
{
    enum Type { numInt, numFloat, boolean, str, list, mapT, null };

    union Value
    {
        int intVal;
        double &floatVal;
        bool boolVal;
        string stringVal;
        vector<Value> listVal;
        map<Value, Value> mapVal;

        Value() {}
        Value(int val) : intVal(val) {}
        Value(double val) : floatVal(val) {}
        Value(bool val) : boolVal(val) {}
        Value(string val) : stringVal(val) {}
        Value(vector<Value> val) : listVal(val) {}
        Value(map<Value, Value> val) : mapVal(val) {}
        ~Value() {}
    };

    Type type;
    Value value;

    Variable(const Variable &other) : type(other.type) { memcpy(&value, &other.value, sizeof(Value)); }

    Variable(int val) : type(numInt), value(val) {}
    Variable(double val) : type(numFloat), value(val) {}
    Variable(bool val) : type(boolean), value(val) {}
    Variable(string val) : type(str), value(val) {}
    Variable(vector<Value> val) : type(list), value(val) {}
    Variable(map<Value, Value> val) : type(mapT), value(val) {}
    Variable(nullptr_t n) : type(null) {}
};

map<Variable, Variable> _List()
{
    map<Variable, Variable> _list;
    _list[Variable("head")] = Variable(nullptr);
    _list[Variable("head")] = Variable(nullptr);
}

int main()
{
    
}