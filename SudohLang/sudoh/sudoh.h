#pragma once
#include <vector>
#include <unordered_map>

void runtimeException(const std::string msg);

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
	Number operator%(const Number& other) const;

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
	//friend class String;
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
		//Val(const Variable& other);
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

	Variable operator+(const Variable& other) const;
	Variable operator-(const Variable& other) const;
	Variable operator*(const Variable& other) const;
	Variable operator/(const Variable& other) const;
	Variable operator%(const Variable& other) const;

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

// +----------------------+
// |   Standard Library   |
// +----------------------+

Variable _print(Variable str);
Variable _printLine(Variable str);
Variable _length(Variable var);
Variable _string(Variable var);
Variable _number(Variable var);
Variable _random();