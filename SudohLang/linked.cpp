#include <iostream>
#include "variable.h"
#define true Bool::t
#define false Bool::f

// stdLib
std::ostream& operator<<(std::ostream& os, const Variable& var)
{
    switch (var.type)
    {
    case Variable::number:
        //os << var.numVal;
        break;
    case Variable::str:
        break;
    case Variable::boolean:
        break;
    case Variable::list:
        break;
    case Variable::mapT:
        break;
    }
    return os;
}

Variable _print(const Variable& _str)
{
    std::cout << _str;
    return nullptr;
}
// end stdLib

/*Variable _List()
{
    Variable _list = new Map {
        { _list["head"], nullptr },
        { _list["size"], Number(0) }
    };
    return _list;
}

Variable _addNode(Variable& _list, Variable& _num)
{
    Variable _node = new Map {
        { Variable("val"), _num },
        { Variable("next"), nullptr }
    };

    if (_list["size"] == Number(0))
    {
        _list["head"] = _node;
    }
    else
    {
        Variable _curr = _list["head"];
        while (_curr["next"] != nullptr)
        {
            _curr = _curr["next"];
        }

        _curr["next"] = _node;
    }
    return nullptr;
}

Variable _printList(Variable& _list)
{
    Variable _curr = _list["head"];
    _print("[");
    while (_curr != nullptr)
    {
        _print(_curr["val"] + ",");
        _curr = _curr["next"];
    }
    _print("]\n");
    return nullptr;
}*/

int main()
{
    /*Variable _list = _List();
    Variable __tmp0 = Number(1);
    _addNode(_list, __tmp0);
    Variable __tmp1 = Number(2);
    _addNode(_list, __tmp1);
    Variable __tmp2 = Number(3);
    _addNode(_list, __tmp2);
    _printList(_list);*/
    
    return 0;
}