#include <iostream>
#include "variable.h"

// stdLib
Variable _print(const Variable& _str)
{
    return nullptr;
}

// end stdLib

Variable _List()
{
    Variable _list = Map();
    _list["head"] = nullptr;
    _list["size"] = Number(0);
    return _list;
}

Variable _addNode(Variable& _list, Variable& _num)
{
    Variable _node = Map();
    _node["val"] = _num;
    _node["next"] = nullptr;

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
    std::cout << "[";
    while (_curr != nullptr)
    {
        _print(_curr["val"] + ",");
        _curr = _curr["next"];
    }
    std::cout << "]" << std::endl;
    return nullptr;
}

/*int main()
{
    Variable _list = _List();
    Variable __tmp0 = Number(1);
    _addNode(_list, __tmp0);
    Variable __tmp1 = Number(2);
    _addNode(_list, __tmp1);
    Variable __tmp2 = Number(3);
    _addNode(_list, __tmp2);
    _printList(_list);
}*/