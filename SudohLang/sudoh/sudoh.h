#pragma once
#include <vector>
#include <unordered_map>

void runtimeException(const std::string msg);

struct Variable;
typedef std::vector<Variable> List;
struct VariableHash
{
	size_t operator()(const Variable& v) const;
};
typedef std::unordered_map<Variable, Variable, VariableHash> Map;

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
	Number operator%(const Number& other) const;

	void operator+=(const Number& other);
	void operator-=(const Number& other);
	void operator*=(const Number& other);
	void operator/=(const Number& other);
	void operator%=(const Number& other);

	bool operator==(const Number& other) const;
	bool operator!=(const Number& other) const;
	bool operator<(const Number& other) const;
	bool operator<=(const Number& other) const;
};

template <typename T>
struct Ref
{
	T val;
	int refCount;
	Ref(T v) : refCount(1), val(v) {}
};

enum class Type { number, boolean, string, list, map, nul, charRef };
class Variable
{
	friend Variable f_length(Variable var);
	friend Variable f_integer(Variable var);
	friend Variable f_remove(Variable list, Variable index);
	friend Variable f_append(Variable list, Variable value);
	friend Variable f_insert(Variable list, Variable index, Variable value);
	friend class VariableHash;

	Type type;
	union Val
	{
		Number numVal;
		bool boolVal;
		std::string stringVal;
		//std::shared_ptr<List> lRef;
		Ref<List>* listRef;
		Ref<Map>* mapRef;

		Val();
		Val(Number val);
		Val(bool val);
		Val(std::string val);
		Val(Ref<List>* val);
		Val(Ref<Map>* val);
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

	Variable operator+(const Variable& other) const;
	Variable operator-(const Variable& other) const;
	Variable operator*(const Variable& other) const;
	Variable operator/(const Variable& other) const;
	Variable operator%(const Variable& other) const;

	void operator+=(const Variable& other);
	void operator-=(const Variable& other);
	void operator*=(const Variable& other);
	void operator/=(const Variable& other);
	void operator%=(const Variable& other);

	Variable& operator=(const Variable& other);

	bool operator==(const Variable& other) const;
	bool operator!=(const Variable& other) const;
	bool operator<(const Variable& other) const;
	bool operator<=(const Variable& other) const;
	bool operator>(const Variable& other) const;
	bool operator>=(const Variable& other) const;

	Variable& operator[](const Variable& index);
	Variable at(const Variable& index) const;

	explicit operator bool() const;

	class VariableIterator
	{
		Variable* container;
		std::string::const_iterator stringIt;
		List::iterator listIt;
		Map::iterator mapIt;

	public:
		VariableIterator(Variable* var, bool begin);
		void operator++();
		Variable operator*();
		bool operator!=(const VariableIterator& other);
	};

	VariableIterator begin();
	VariableIterator end();
};

#define null Variable()

using var = Variable;

// +----------------------+
// |   Standard Library   |
// +----------------------+

Variable f_print(Variable str);
Variable f_printLine(Variable str);
Variable f_length(Variable var);
Variable f_string(Variable var);
Variable f_integer(Variable var);
Variable f_random();
Variable f_remove(Variable list, Variable index);
Variable f_append(Variable list, Variable value);
Variable f_insert(Variable list, Variable index, Variable value);
