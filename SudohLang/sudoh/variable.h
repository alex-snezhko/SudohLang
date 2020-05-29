#pragma once
#include "number.h"
#include <vector>
#include <unordered_map>

template <typename T>
struct Ref
{
	T val;
	int refCount;
	Ref(T v) : refCount(1), val(v) {}
};

enum class Type { number, boolean, string, list, map, null, charRef };
class Variable
{
	struct VariableHash
	{
		size_t operator()(const Variable& v) const;
	};

	typedef std::vector<Variable> List;
	typedef std::unordered_map<Variable, Variable, VariableHash> Map;

	// standard library functions which have access to Variable members
	friend Variable f_length(Variable var);
	friend Variable f_number(Variable str);
	friend Variable f_integer(Variable var);
	friend Variable f_ascii(Variable num);
	friend Variable f_random(Variable range);
	friend Variable f_remove(Variable list, Variable index);
	friend Variable f_append(Variable list, Variable value);
	friend Variable f_insert(Variable list, Variable index, Variable value);
	friend Variable f_removeLast(Variable list);
	friend Variable f_substring(Variable str, Variable begin, Variable end);
	friend double floatVal(std::string which, std::string procedure, const Variable& var);

	friend class VariableHash;

	Type type;
	union Val
	{
		Number numVal;
		bool boolVal;
		std::string stringVal;
		Ref<List>* listRef; // TODO debug that ref count checking works properly
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

const Variable null = Variable();
