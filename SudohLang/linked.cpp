#include "linked.h"

var f_List()
{
	var _list = var(Map{ { var(std::string("head")), null }, { var(std::string("size")), var(0) } });
	return _list;
	return null;
}

var f_addNode(var _list, var _val)
{
	var _node = var(Map{ { var(std::string("val")), _val }, { var(std::string("next")), null } });
	if (_list.at(var(std::string("size"))) == var(0))
	{
		_list[var(std::string("head"))] = _node;
	}
	else
	{
		var _curr = _list.at(var(std::string("head")));
		while (_curr.at(var(std::string("next"))) != null)
		{
			_curr = _curr.at(var(std::string("next")));
		}
		_curr[var(std::string("next"))] = _node;
	}
	_list[var(std::string("size"))] += var(1);
	return null;
}

var f_printList(var _list)
{
	var _curr = _list.at(var(std::string("head")));
	f_print(var(std::string("[")));
	while (!(_curr == null))
	{
		f_print(_curr.at(var(std::string("val"))) + var(std::string(",")));
		_curr = _curr.at(var(std::string("next")));
	}
	f_printLine(var(std::string("]")));
	return null;
}

var f_sort(var _arr)
{
	var _n = f_length(_arr);
	for (var _i = var(0); _i <= _n - var(2); _i += 1)
	{
		for (var _j = var(0); _j <= _n - _i - var(2); _j += 1)
		{
			if (_arr.at(_j) > _arr.at(_j + var(1)))
			{
				var _temp = _arr.at(_j);
				_arr[_j] = _arr.at(_j + var(1));
				_arr[_j + var(1)] = _temp;
			}
		}
	}
	return null;
}

