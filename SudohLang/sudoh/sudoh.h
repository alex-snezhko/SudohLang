#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

class SudohRuntimeException
{
	std::string msg;

public:
	SudohRuntimeException(std::string message) : msg(message) {}
	const char* what() const throw()
	{
		return msg.c_str();
	}
};

enum class Type { number, boolean, string, list, map, nul };

struct Number
{
	union
	{
		int intVal;
		double floatVal;
	};
	bool isInt;

	double getFloatVal() const { return isInt ? (double)intVal : floatVal; }

	Number(int i) : intVal(i), isInt(true) {}
	Number(double f) : floatVal(f), isInt(false) {}

	Number operator+(const Number& other) const
	{
		return isInt && other.isInt ?
			Number(intVal + other.intVal) :
			Number(getFloatVal() + other.getFloatVal());
	}

	Number operator-(const Number& other) const
	{
		return isInt && other.isInt ?
			Number(intVal - other.intVal) :
			Number(getFloatVal() - other.getFloatVal());
	}

	Number operator*(const Number& other) const
	{
		return isInt && other.isInt ?
			Number(intVal * other.intVal) :
			Number(getFloatVal() * other.getFloatVal());
	}

	Number operator/(const Number& other) const
	{
		if (other.getFloatVal() == 0)
		{
			throw SudohRuntimeException("division by zero");
		}
		return isInt && other.isInt && (intVal % other.intVal == 0) ?
			Number(intVal / other.intVal) :
			Number(getFloatVal() / other.getFloatVal());
	}

	bool operator==(const Number& other) const
	{
		return isInt && other.isInt ?
			intVal == other.intVal :
			getFloatVal() == other.getFloatVal();
	}

	bool operator!=(const Number& other) const
	{
		return !operator==(other);
	}

	bool operator<(const Number& other) const
	{
		return isInt && other.isInt ?
			intVal < other.intVal :
			getFloatVal() < other.getFloatVal();
	}

	bool operator<=(const Number& other) const
	{
		return isInt && other.isInt ?
			intVal <= other.intVal :
			getFloatVal() <= other.getFloatVal();
	}
};

struct Variable;

struct Hash
{
	size_t operator()(const Variable& v) const
	{
		return 0;
	}
};
typedef std::string String;
typedef std::vector<Variable> List;
typedef std::unordered_map<Variable, Variable, Hash> Map;

struct Ref
{
	int refCount;
	Ref(int r) : refCount(r) {}
};

struct ListRef : public Ref
{
	List listVal;
	ListRef(List l) : Ref(1), listVal(l) {}
};

struct MapRef : public Ref
{
	Map mapVal;
	MapRef(Map m) : Ref(1), mapVal(m) {}
};

struct Variable
{
	Type type;
	String typeString() const
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

	union Val
	{
		Number numVal;
		bool boolVal;
		String* stringRef;
		Ref* sharedRef;

		Val(Number val) : numVal(val) {}
		Val(bool val) : boolVal(val) {}
		Val(String val) : stringRef(new String(val)) {}
		Val(Ref* val) : sharedRef(val) {}
	} val;

	Variable() : type(Type::nul), val(false) {}
	Variable(int n) : type(Type::number), val(Number(n)) {}
	Variable(double n) : type(Type::number), val(Number(n)) {}
	Variable(Number n) : type(Type::number), val(n) {}
	Variable(bool b) : type(Type::boolean), val(b) {}
	//Variable(const char* s) : type(Type::string), val(String(s)) {}
	Variable(String s) : type(Type::string), val(s) {}
	Variable(List l) : type(Type::list), val(new ListRef(l)) {}
	Variable(Map m) : type(Type::map), val(new MapRef(m)) {}

	void setValue(const Variable& other)
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
			val.stringRef = new String(*other.val.stringRef);
			break;
		case Type::list:
		case Type::map:
			val.sharedRef = other.val.sharedRef;
			val.sharedRef->refCount++;
			break;
		}
	}

	Variable(const Variable& other) : type(other.type), val(other.val) { setValue(other); }

	void freeMem()
	{
		if (type == Type::list || type == Type::map)
		{
			if (--val.sharedRef->refCount == 0)
			{
				delete val.sharedRef;
			}
		}
		if (type == Type::string)
		{
			delete val.stringRef;
		}
	}

	~Variable() { freeMem(); }

	String toString() const
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
			return *val.stringRef;
		case Type::list:
			return "list";//TODO
		case Type::map:
			return "map";//TODO
		case Type::nul:
			return "null";
		}
	}

	Variable operator+(const Variable& other) const
	{
		switch (type)
		{
		case Type::number:
			if (other.type == Type::string)
			{
				return toString() + *other.val.stringRef;
			}
			if (other.type == Type::number)
			{
				return val.numVal + other.val.numVal;
			}
			break;
			
		case Type::string:
			return *val.stringRef + other.toString();
		case Type::list:
			return Variable();//TODO
		}

		throw SudohRuntimeException("illegal operation '+' between types " + typeString() + " and " + other.typeString());
	}

	Variable operator-(const Variable& other) const
	{
		if (type == Type::number && other.type == Type::number)
		{
			return val.numVal - other.val.numVal;
		}

		throw SudohRuntimeException("illegal operation '-' between types " + typeString() + " and " + other.typeString());
	}

	Variable& operator=(const Variable& other)
	{
		if (this != &other)
		{
			freeMem();
			type = other.type;
			setValue(other);
		}
		return *this;
	}

	bool operator==(const Variable& other) const
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
			return *val.stringRef == *other.val.stringRef;
		case Type::list:
		case Type::map:
			return val.sharedRef == other.val.sharedRef;
		}
		return false;
	}

	bool operator!=(const Variable& other) const
	{
		return !operator==(other);
	}

	bool operator<(const Variable& other) const
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
			return *val.stringRef < *other.val.stringRef;
		}

		throw SudohRuntimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	bool operator<=(const Variable& other) const
	{
		if (other.type != type)
		{
			throw SudohRuntimeException("illegal comparison between types " +  typeString() + " and " + other.typeString());
		}

		switch (other.type)
		{
		case Type::number:
			return val.numVal <= other.val.numVal;
		case Type::string:
			return *val.stringRef <= *other.val.stringRef;
		}

		throw SudohRuntimeException("illegal comparison between types " + typeString() + " and " + other.typeString());
	}

	bool operator>(const Variable& other) const
	{
		return !operator<=(other);
	}

	bool operator>=(const Variable& other) const
	{
		return !operator<(other);
	}

	Variable& operator[](const Variable& index)
	{
		switch (type)
		{
		case Type::string:
		{
			if (index.type != Type::number || !index.val.numVal.isInt)
			{
				throw SudohRuntimeException("specified string index is not an integer");
			}
			int i = index.val.numVal.intVal;
			if (i < 0 || i >= val.stringRef->length())
			{
				throw SudohRuntimeException("specified string index out of bounds");
			}
			const char c[2]{ (*val.stringRef)[i], '\0' };
			Variable st = c;
			return st; // Sudoh strings immutable; trying to set value of character in string won't even transpile
		}
		case Type::list:
		{
			if (index.type != Type::number || !index.val.numVal.isInt)
			{
				throw SudohRuntimeException("specified list index is not an integer");
			}
			int i = index.val.numVal.intVal;
			if (i < 0 || i >= ((ListRef*)val.sharedRef)->listVal.size())
			{
				throw SudohRuntimeException("specified list index out of bounds");
			}
			return ((ListRef*)val.sharedRef)->listVal[i];
		}
		case Type::map:
		{
			Map& m = ((MapRef*)(val.sharedRef))->mapVal;
			return m[index];
		}
		}

		throw SudohRuntimeException("cannot index into type " + typeString());
	}
};

#define null Variable()

// +----------------------+
// |   Standard Library   |
// +----------------------+

Variable _print(Variable& str)
{
	switch (str.type)
	{
	case Type::number:
		std::cout << (str.val.numVal.isInt ? str.val.numVal.intVal : str.val.numVal.floatVal);
		break;
	case Type::string:
		std::cout << *str.val.stringRef;
		break;
	case Type::boolean:
		std::cout << str.val.boolVal;
		break;
	case Type::list:
		// TODO
		break;
	case Type::map:
		// TODO
		break;
	}
	return null;
}

Variable _printLine(Variable& str)
{
	_print(str);
	std::cout << std::endl;
	return null;
}

Variable _length(Variable& var)
{
	switch (var.type)
	{
	case Type::list:
		return (int)((ListRef*)var.val.sharedRef)->listVal.size();
	case Type::map:
		return (int)((MapRef*)var.val.sharedRef)->mapVal.size();
	case Type::string:
		return (int)var.val.stringRef->length();
	}
	throw SudohRuntimeException("cannot take length of type " + var.typeString());
}

Variable _string(Variable& var)
{
	return var.toString();
}

Variable _number(Variable& var)
{
	//switch (var.type)
	//{
		//TODO
	//}
	return null;
}

Variable _random()
{
	return rand(); // TODO add srand to beginning of transpiled output
}
