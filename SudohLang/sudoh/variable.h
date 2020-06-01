#pragma once
#include <vector>
#include <unordered_map>
#include <memory>

// enum that is used to keep track of the type of a variable
enum class Type { number, boolean, string, list, map, null, charRef };

// Boolean pseudo-type that is used to avoid ambiguity for Variable constructor;
// for example, so that 'Variable(0)' is not ambiguated between using bool or double constructor
enum class Bool { t, f };

class Variable
{
	struct VariableHash
	{
		size_t operator()(const Variable& v) const;
	};

public:
	typedef std::vector<Variable> List;
	typedef std::unordered_map<Variable, Variable, VariableHash> Map;

private:
	// standard library functions which have access to Variable members
	friend Variable f_length(Variable var);
	friend Variable f_remove(Variable list, Variable index);
	friend Variable f_range(Variable indexable, Variable begin, Variable end);

	Type type;
	union Val
	{
		double numVal;
		bool boolVal;
		std::string stringVal;
		std::shared_ptr<List> listRef;
		std::shared_ptr<Map> mapRef;

		Val();
		Val(double val);
		Val(bool val);
		Val(std::string val);
		Val(std::shared_ptr<List> val);
		Val(std::shared_ptr<Map> val);
		~Val();
	} val;

	void freeMem();
	void setValue(const Variable& other);

public:

	static bool stringCheck(const Variable& var, std::string& out);
	static bool indexCheck(const Variable& var, size_t& out);
	static bool numCheck(const Variable& var, double& out);
	static bool listCheck(const Variable& var, List*& out);

	Variable();
	Variable(double n);
	Variable(Bool b);
	Variable(std::string s);
	Variable(std::shared_ptr<List> l);
	Variable(std::shared_ptr<Map> m);

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
