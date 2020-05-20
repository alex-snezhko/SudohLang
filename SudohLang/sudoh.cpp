#include "sudoh.h"

Variable _List();
Variable _addNode(Variable, Variable);
Variable _printList(Variable);

Variable _List()
{
	Variable _list = Map{ { std::string("head"), null }, { std::string("size"), 0 } };
	return _list;
	return null;
}

Variable _addNode(Variable _list, Variable _val)
{
	Variable _node = Map{ { std::string("val"), _val }, { std::string("next"), null } };
	if (_list.at(std::string("size")) == 0)
	{
		_list[std::string("head")] = _node;
	}
	else
	{
		Variable _curr = _list.at(std::string("head"));
		while (_curr.at(std::string("next")) != null)
		{
			_curr = _curr.at(std::string("next"));
		}
		_curr[std::string("next")] = _node;
	}
	_list[std::string("size")] = _list.at(std::string("size")) + 1;
	return null;
}

Variable _printList(Variable _list)
{
	Variable _curr = _list.at(std::string("head"));
	_print(std::string("["));
	while (!(_curr == null))
	{
		_print(_curr.at(std::string("val")) + std::string(","));
		_curr = _curr.at(std::string("next"));
	}
	_printLine(std::string("]"));
	return null;
}

int main()
{
	Variable _list = _List();
	_addNode(_list, 1);
	_addNode(_list, 2);
	_addNode(_list, 3);
	_printList(_list);
}
