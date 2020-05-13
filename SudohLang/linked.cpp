#include "sudoh.h"

Variable swap(Variable&, Variable&);
Variable sort(Variable&);

Variable _swap(Variable& _a, Variable& _b)
{
    Variable _temp = _a;
    _a = _b;
    _b = _temp;
    return null;
}
Variable _sort(Variable& _arr)
{
    Variable _n = _length(_arr);
    for (Variable _i = 0; _i <= _n - 2; _i = _i + 1)
    {
        for (Variable _j = 0; _j <= _n - _i - 2; _j = _j + 1)
        {
            if (_arr[_j] > _arr[_j + 1])
            {
                _swap(_arr[_j], _arr[_j + 1]);
            }
        }
    }
    return null;
}

/*int main()
{
    Variable _arr = List{ 3, 10, 4, 5, 2, 7, 8, 0, 1 };
    _sort(_arr);
    for (Variable _i = 0; _i <= _length(_arr) - 1; _i = _i + 1)
    {
        _print(_arr[_i]);
    }
}*/
