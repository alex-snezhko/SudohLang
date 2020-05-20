#include "sudoh.h"
#include <string>
#include <iostream>
#include <memory>

void runtimeException(const std::string msg)
{
	std::string output = "Runtime exception: " + msg + "; terminating program";
	std::cout << "\n+";
	for (int i = 0; i < output.length() + 4; i++)
	{
		std::cout << "-";
	}
	std::cout << "+\n|  " << output << "  |\n+";
	for (int i = 0; i < output.length() + 4; i++)
	{
		std::cout << "-";
	}
	std::cout << "+\n";
	exit(0);
}

std::string Variable::typeString() const
{
	switch (type)
	{
	case Type::number:
		return "number";
	case Type::boolean:
		return "boolean";
	case Type::string:
		return "string";
	case Type::list:
		return "list";
	case Type::map:
		return "map";
	case Type::nul:
		return "null";
	}
}

Variable::Val::Val() : boolVal(false) {}
Variable::Val::Val(Number val) : numVal(val) {}
Variable::Val::Val(bool val) : boolVal(val) {}
Variable::Val::Val(std::string val) : stringVal(val) {}
Variable::Val::Val(Ref<List>* val) : listRef(val) {}
Variable::Val::Val(Ref<Map>* val) : mapRef(val) {}
Variable::Val::~Val() {}

Variable::Variable() : type(Type::nul) {}
Variable::Variable(int n) : type(Type::number), val(Number(n)) {}
Variable::Variable(double n) : type(Type::number), val(Number(n)) {}
Variable::Variable(Number n) : type(Type::number), val(n) {}
Variable::Variable(bool b) : type(Type::boolean), val(b) {}
Variable::Variable(std::string s) : type(Type::string), val(s) {}
Variable::Variable(List l) : type(Type::list), val(new Ref<List>(l)) {}
Variable::Variable(Map m) : type(Type::map), val(new Ref<Map>(m)) {}

Variable::Variable(const Variable& other) : type(other.type) { setValue(other); }

Variable::~Variable() { freeMem(); }

void Variable::setValue(const Variable& other)
{
	switch (other.type)
	{
	case Type::number:
		val.numVal = other.val.numVal;
		break;
	case Type::boolean:
		val.boolVal = other.val.boolVal;
		break;
	case Type::string:
		new(&val.stringVal) std::string(other.val.stringVal);
		break;
	case Type::list:
		val.listRef = other.val.listRef;
		val.listRef->refCount++;
		break;
	case Type::map:
		val.mapRef = other.val.mapRef;
		val.mapRef->refCount++;
		break;
	}
}

void Variable::freeMem()
{
	if (type == Type::list)
	{
		if (--val.listRef->refCount == 0)
		{
			delete val.listRef;
		}
	}
	if (type == Type::map)
	{
		if (--val.mapRef->refCount == 0)
		{
			delete val.mapRef;
		}
	}
	else if (type == Type::string)
	{
		val.stringVal.~basic_string();
	}
}

std::string Variable::toString() const
{
	switch (type)
	{
	case Type::number:
		if (val.numVal.isInt)
		{
			return std::to_string(val.numVal.intVal);
		}
		return std::to_string(val.numVal.floatVal);
	case Type::boolean:
		return std::to_string(val.boolVal);
	case Type::string:
		return val.stringVal;
	case Type::list:
	{
		std::string contents = "[ ";
		bool first = true;
		for (const Variable& e : val.listRef->val)
		{
			if (!first)
			{
				contents += ", ";
			}
			contents += e.toString();
			first = false;
		}
		contents += " ]";
		return contents;
	}
	case Type::map:
	{
		std::string contents = "{ ";
		bool first = true;
		for (std::pair<const Variable, Variable>& e : val.mapRef->val)
		{
			if (!first)
			{
				contents += ", ";
			}
			contents += e.first.toString() + " <- " + e.second.toString();
			first = false;
		}
		contents += " }";
		return contents;
	}
	case Type::nul:
		return "null";
	}
}

int Variable::length() const
{
	switch (type)
	{
	case Type::list:
		return val.listRef->val.size();
	case Type::map:
		return val.mapRef->val.size();
	case Type::string:
		return val.stringVal.length();
	}
	runtimeException("cannot take length of type " + typeString());
}

Variable Variable::operator+(const Variable& other) const
{
	switch (type)
	{
	case Type::number:
		if (other.type == Type::string)
		{
			return toString() + other.val.stringVal;
		}
		if (other.type == Type::number)
		{
			return val.numVal + other.val.numVal;
		}
		break;

	case Type::string:
		return val.stringVal + other.toString();
	case Type::list:
	{
		List newList = val.listRef->val;
		newList.push_back(other);
		return newList;
	}
	}

	runtimeException("illegal operation '+' between types " + typeString() + " and " + other.typeString());
}

Variable Variable::operator-(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal - other.val.numVal;
	}

	runtimeException("illegal operation '-' between types " + typeString() + " and " + other.typeString());
}

Variable Variable::operator*(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal * other.val.numVal;
	}

	runtimeException("illegal operation '*' between types " + typeString() + " and " + other.typeString());
}

Variable Variable::operator/(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal / other.val.numVal;
	}

	runtimeException("illegal operation '/' between types " + typeString() + " and " + other.typeString());
}

Variable Variable::operator%(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal % other.val.numVal;
	}

	runtimeException("illegal operation 'mod' between types " + typeString() + " and " + other.typeString());
}

Variable& Variable::operator=(const Variable& other)
{
	if (this != &other)
	{
		freeMem();
		type = other.type;
		setValue(other);
	}
	return *this;
}

bool Variable::operator==(const Variable& other) const
{
	if (type == Type::nul || other.type == Type::nul)
	{
		return type == other.type;
	}

	if (other.type != type)
	{
		runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return val.numVal == other.val.numVal;
	case Type::boolean:
		return val.boolVal == other.val.boolVal;
	case Type::string:
		return val.stringVal == other.val.stringVal;
	case Type::list:
		return val.listRef == other.val.listRef;
	case Type::map:
		return val.mapRef == other.val.mapRef;
	}
	return false;
}

bool Variable::operator!=(const Variable& other) const
{
	return !operator==(other);
}

bool Variable::operator<(const Variable& other) const
{
	if (other.type != type)
	{
		runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return val.numVal < other.val.numVal;
	case Type::string:
		return val.stringVal < other.val.stringVal;
	}

	runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
}

bool Variable::operator<=(const Variable& other) const
{
	if (other.type != type)
	{
		runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return val.numVal <= other.val.numVal;
	case Type::string:
		return val.stringVal <= other.val.stringVal;
	}

	runtimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
}

bool Variable::operator>(const Variable& other) const
{
	return !operator<=(other);
}

bool Variable::operator>=(const Variable& other) const
{
	return !operator<(other);
}

// will only appear on left-hand side of expression
Variable& Variable::operator[](const Variable& index)
{
	int i;
	switch (type)
	{
	case Type::string:
		runtimeException("illegal attempt to modify string");
	case Type::list:
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			runtimeException("specified list index is not an integer");
		}
		i = index.val.numVal.intVal;
		if (i < 0 || i >= val.listRef->val.size())
		{
			runtimeException("specified list index out of bounds");
		}
		return val.listRef->val[i];
	case Type::map:
		return val.mapRef->val[index];
	}

	runtimeException("cannot index into type " + typeString());
}

// non-reference equivalent of [] operator; for accesses rather than modifications
Variable Variable::at(const Variable& index) const
{
	int i;
	switch (type)
	{
	case Type::string:
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			runtimeException("specified string index is not an integer");
		}
		i = index.val.numVal.intVal;
		if (i < 0 || i >= val.stringVal.length())
		{
			runtimeException("specified string index out of bounds");
		}
		return std::string(1, val.stringVal[i]);
	case Type::list:
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			runtimeException("specified list index is not an integer");
		}
		i = index.val.numVal.intVal;
		if (i < 0 || i >= val.listRef->val.size())
		{
			runtimeException("specified list index out of bounds");
		}
		return val.listRef->val[i];
	case Type::map:
		return val.mapRef->val[index];
	}
	
	runtimeException("cannot index into type " + typeString());
}

Variable::operator bool() const
{
	if (type == Type::boolean)
	{
		return val.boolVal;
	}
	runtimeException("expected boolean type");
}

// +-------------------------------------+
// |   VariableIterator implementation   |
// +-------------------------------------+

Variable::VariableIterator::VariableIterator(Variable* var, bool begin) : container(var)
{
	switch (var->type)
	{
	case Type::string:
		stringIt = begin ? var->val.stringVal.begin() : var->val.stringVal.end();
		break;
	case Type::list:
		listIt = begin ? var->val.listRef->val.begin() : var->val.listRef->val.end();
		break;
	case Type::map:
		mapIt = begin ? var->val.mapRef->val.begin() : var->val.mapRef->val.end();
		break;
	default:
		runtimeException("cannot iterate over type " + var->typeString());
	}
}

void Variable::VariableIterator::operator++()
{
	switch (container->type)
	{
	case Type::string:
		stringIt++;
		break;
	case Type::list:
		listIt++;
		break;
	case Type::map:
		mapIt++;
		break;
	}
}

Variable Variable::VariableIterator::operator*()
{
	switch (container->type)
	{
	case Type::string:
		return std::string(1, *stringIt);
	case Type::list:
		return *listIt;
	case Type::map:
		return mapIt->first;
	}
}

bool Variable::VariableIterator::operator!=(const VariableIterator& other)
{
	switch (container->type)
	{
	case Type::string:
		return stringIt != other.stringIt;
	case Type::list:
		return listIt != other.listIt;
	case Type::map:
		return mapIt != other.mapIt;
	}
}

Variable::VariableIterator Variable::begin()
{
	return VariableIterator(this, true);
}

Variable::VariableIterator Variable::end()
{
	return VariableIterator(this, false);
}
