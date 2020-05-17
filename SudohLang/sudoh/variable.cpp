#include "sudoh.h"
#include <string>

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
Variable::Val::Val(Ref* val) : sharedRef(val) {}
Variable::Val::~Val() {}

Variable::Variable() : type(Type::nul) {}
Variable::Variable(int n) : type(Type::number), val(n) {}
Variable::Variable(double n) : type(Type::number), val(n) {}
Variable::Variable(Number n) : type(Type::number), val(n) {}
Variable::Variable(bool b) : type(Type::boolean), val(b) {}
Variable::Variable(std::string s) : type(Type::string), val(s) {}
Variable::Variable(List l) : type(Type::list), val(new ListRef(l)) {}
Variable::Variable(Map m) : type(Type::map), val(new MapRef(m)) {}

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
		val.stringVal = other.val.stringVal;
		break;
	case Type::list:
	case Type::map:
		val.sharedRef = other.val.sharedRef;
		val.sharedRef->refCount++;
		break;
	}
}

void Variable::freeMem()
{
	if (type == Type::list || type == Type::map)
	{
		if (--val.sharedRef->refCount == 0)
		{
			delete val.sharedRef;
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
		return "list";//TODO
	case Type::map:
		return "map";//TODO
	case Type::nul:
		return "null";
	}
}

int Variable::length() const
{
	switch (type)
	{
	case Type::list:
		return (int)((ListRef*)val.sharedRef)->listVal.size();
	case Type::map:
		return (int)((MapRef*)val.sharedRef)->mapVal.size();
	case Type::string:
		return (int)val.stringVal.length();
	}
	throw SudohRuntimeException("cannot take length of type " + typeString());
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
		return Variable();//TODO
	}

	throw SudohRuntimeException("illegal operation '+' between types " + typeString() + " and " + other.typeString());
}

Variable Variable::operator-(const Variable& other) const
{
	if (type == Type::number && other.type == Type::number)
	{
		return val.numVal - other.val.numVal;
	}

	throw SudohRuntimeException("illegal operation '-' between types " + typeString() + " and " + other.typeString());
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
		throw SudohRuntimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
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
	case Type::map:
		return val.sharedRef == other.val.sharedRef;
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
		throw SudohRuntimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return val.numVal < other.val.numVal;
	case Type::string:
		return val.stringVal < other.val.stringVal;
	}

	throw SudohRuntimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
}

bool Variable::operator<=(const Variable& other) const
{
	if (other.type != type)
	{
		throw SudohRuntimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	switch (other.type)
	{
	case Type::number:
		return val.numVal <= other.val.numVal;
	case Type::string:
		return val.stringVal <= other.val.stringVal;
	}

	throw SudohRuntimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
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
Variable& Variable::operator[](const Variable& index) const
{
	int i;
	switch (type)
	{
	case Type::string:
		throw SudohRuntimeException("illegal attempt to modify string");
	case Type::list:
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			throw SudohRuntimeException("specified list index is not an integer");
		}
		i = index.val.numVal.intVal;
		if (i < 0 || i >= ((ListRef*)val.sharedRef)->listVal.size())
		{
			throw SudohRuntimeException("specified list index out of bounds");
		}
		return ((ListRef*)val.sharedRef)->listVal[i];
	case Type::map:
		return ((MapRef*)(val.sharedRef))->mapVal[index];
	}

	throw SudohRuntimeException("cannot index into type " + typeString());
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
			throw SudohRuntimeException("specified string index is not an integer");
		}
		i = index.val.numVal.intVal;
		if (i < 0 || i >= val.stringVal.length())
		{
			throw SudohRuntimeException("specified string index out of bounds");
		}
		return std::to_string(val.stringVal[i]);
	case Type::list:
		if (index.type != Type::number || !index.val.numVal.isInt)
		{
			throw SudohRuntimeException("specified list index is not an integer");
		}
		i = index.val.numVal.intVal;
		if (i < 0 || i >= ((ListRef*)val.sharedRef)->listVal.size())
		{
			throw SudohRuntimeException("specified list index out of bounds");
		}
		return ((ListRef*)val.sharedRef)->listVal[i];
	case Type::map:
		return ((MapRef*)(val.sharedRef))->mapVal[index];
	}
	
	throw SudohRuntimeException("cannot index into type " + typeString());
}

// +-------------------------------------+
// |   VariableIterator implementation   |
// +-------------------------------------+

void VariableIterator::operator++()
{

}

Variable& VariableIterator::operator*()
{
	return *curr;
}
