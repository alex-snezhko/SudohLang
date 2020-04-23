#include <iostream>
#include "variable.h"

#define null Variable()

// stdLib
std::ostream& operator<<(std::ostream& os, const Variable& var)
{
    switch (var.type)
    {
    case Type::number:
        if (var.val.numVal.isInt)
        {
            os << var.val.numVal.intVal;
        }
        else
        {
            os << var.val.numVal.floatVal;
        }
        break;
    case Type::str:
        os << *var.val.stringRef;
        break;
    case Type::boolean:
        os << var.val.boolVal;
        break;
    case Type::list:
        break;
    case Type::mapT:
        break;
    }
    return os;
}

Variable _print(const Variable& _str)
{
    std::cout << _str;
    return null;
}
// end stdLib

Variable _List()
{
    Variable _list = Map {
        { "head", null },
        { "size", 0 }
    };
    return _list;
}

Variable _addNode(Variable& _list, Variable& _num)
{
    Variable _node = Map {
        { Variable("val"), _num },
        { Variable("next"), null }
    };

    if (_list["size"] == 0)
    {
        _list["head"] = _node;
    }
    else
    {
        Variable _curr = _list["head"];
        while (_curr["next"] != null)
        {
            _curr = _curr["next"];
        }

        _curr["next"] = _node;
    }
    _list["size"] = _list["size"] + 1;
    return null;
}

Variable _printList(Variable& _list)
{
    Variable _curr = _list["head"];
    _print("[");
    while (_curr != null)
    {
        _print(_curr["val"] + ",");
        _curr = _curr["next"];
    }
    _print("]");
    return null;
}

/*int main()
{
    Variable _list = _List();
    Variable __tmp0 = 1;
    _addNode(_list, __tmp0);
    Variable __tmp1 = 2;
    _addNode(_list, __tmp1);
    Variable __tmp2 = 3;
    _addNode(_list, __tmp2);
    _printList(_list);
    return 0;
}*/