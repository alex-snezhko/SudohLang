#include "variable.h"

bool operator<(const Variable& v1, const Variable& v2)
{
    return false;//o1.stringVal < other.stringVal;
}

bool operator==(const Variable& v1, const Variable& v2)
{
    if (v2.type == Variable::null)
    {
        return v1.type == Variable::null;
    }

    if (v2.type != v1.type)
    {
        throw std::exception();
    }

    switch (v1.type)
    {
    case Variable::number:
        //return numVal == other.numVal;
    case Variable::boolean:
        //return boolVal == other.boolVal;
    case Variable::str:
        return *v1.val.stringRef == *v2.val.stringRef;
    case Variable::list:
        //return value.listVal == other.value.listVal;
    case Variable::mapT:
        //return value.mapVal == other.value.mapVal;
    default:
        break;
    }
    return false;
}

bool operator!=(const Variable& v1, const Variable& v2)
{
    return !operator==(v1, v2);
}