#include "sudoh.h"

Variable _sort(Variable);

Variable _sort(Variable _arr)
{
	Variable _n = _length(_arr);
	for (Variable _i = 0; _i <= _n - 2; _i += 1)
	{
		for (Variable _j = 0; _j <= _n - _i - 2; _j += 1)
		{
			if (_arr.at(_j) > _arr.at(_j + 1))
			{
				Variable _temp = _arr.at(_j);
				_arr[_j] = _arr.at(_j + 1);
				_arr[_j + 1] = _temp;
			}
		}
	}
	return null;
}

int main()
{
	Variable _arr = List{  };
	for (Variable _i = 0; _i <= 1000; _i += 1)
	{
		_arr[_i] = _random() % 100;
	}
	_sort(_arr);
	_printLine(_arr);
}
