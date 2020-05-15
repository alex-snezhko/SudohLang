#pragma once
#include <vector>
#include <unordered_map>

class SudohRuntimeException : public std::exception
{
	std::string msg;

public:
	SudohRuntimeException(std::string message) : msg(message) {}
	const char* what() const throw()
	{
		return msg.c_str();
	}
};

struct Variable;
typedef std::vector<Variable> List;
struct Hash
{
	size_t operator()(const Variable& v) const
	{
		return 0;
	}
};
typedef std::unordered_map<Variable, Variable, Hash> Map;

struct Number
{
	union
	{
		int intVal;
		double floatVal;
	};
	bool isInt;

	double getFloatVal() const;

public:
	Number(int i);
	Number(double f);

	Number operator+(const Number& other) const;
	Number operator-(const Number& other) const;
	Number operator*(const Number& other) const;
	Number operator/(const Number& other) const;
	bool operator==(const Number& other) const;
	bool operator!=(const Number& other) const;
	bool operator<(const Number& other) const;
	bool operator<=(const Number& other) const;
};

struct Ref
{
	int refCount;
public:
	Ref();
};

struct ListRef : public Ref
{
	List listVal;
public:
	ListRef(List l);
};

struct MapRef : public Ref
{
	Map mapVal;
public:
	MapRef(Map m);
};


enum class Type { number, boolean, string, list, map, nul };
class Variable
{
	Type type;
	union Val
	{
		Number numVal;
		bool boolVal;
		std::string stringVal;
		Ref* sharedRef;

		Val();
		Val(Number val);
		Val(bool val);
		Val(std::string val);
		Val(Ref* val);
		~Val();
	} val;

	void freeMem();
	void setValue(const Variable& other);

public:
	Variable();
	Variable(int n);
	Variable(double n);
	Variable(Number n);
	Variable(bool b);
	Variable(std::string s);
	Variable(List l);
	Variable(Map m);
	Variable(const Variable& other);

	~Variable();

	std::string typeString() const;
	std::string toString() const;
	int length() const;

	class VariableIterator& begin();
	class VariableIterator& end();

	Variable operator+(const Variable& other) const;
	Variable operator-(const Variable& other) const;
	Variable& operator=(const Variable& other);
	bool operator==(const Variable& other) const;
	bool operator!=(const Variable& other) const;
	bool operator<(const Variable& other) const;
	bool operator<=(const Variable& other) const;
	bool operator>(const Variable& other) const;
	bool operator>=(const Variable& other) const;
	Variable& operator[](const Variable& index) const;
	Variable at(const Variable& index) const;
};

#define null Variable()

class VariableIterator
{
	Variable* container;
	Variable* curr;
public:
	void operator++();
	Variable& operator*();
};

// +----------------------+
// |   Standard Library   |
// +----------------------+

Variable _print(Variable& str);
Variable _printLine(Variable& str);
Variable _length(Variable& var);
Variable _string(Variable& var);
Variable _number(Variable& var);
Variable _random();