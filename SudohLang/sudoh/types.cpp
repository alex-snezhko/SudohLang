#include "sudoh.h"

// +---------------------------+
// |   Number implementation   |
// +---------------------------+

Number::Number(int i) : intVal(i), isInt(true) {}
Number::Number(double f) : floatVal(f), isInt(false) {}

double Number::getFloatVal() const { return isInt ? (double)intVal : floatVal; }

Number Number::operator+(const Number& other) const
{
	return isInt && other.isInt ?
		Number(intVal + other.intVal) :
		Number(getFloatVal() + other.getFloatVal());
}

Number Number::operator-(const Number& other) const
{
	return isInt && other.isInt ?
		Number(intVal - other.intVal) :
		Number(getFloatVal() - other.getFloatVal());
}

Number Number::operator*(const Number& other) const
{
	return isInt && other.isInt ?
		Number(intVal * other.intVal) :
		Number(getFloatVal() * other.getFloatVal());
}

Number Number::operator/(const Number& other) const
{
	if (other.getFloatVal() == 0)
	{
		throw SudohRuntimeException("division by zero");
	}
	return isInt && other.isInt && (intVal % other.intVal == 0) ?
		Number(intVal / other.intVal) :
		Number(getFloatVal() / other.getFloatVal());
}

bool Number::operator==(const Number& other) const
{
	return isInt && other.isInt ?
		intVal == other.intVal :
		getFloatVal() == other.getFloatVal();
}

bool Number::operator!=(const Number& other) const
{
	return !operator==(other);
}

bool Number::operator<(const Number& other) const
{
	return isInt && other.isInt ?
		intVal < other.intVal :
		getFloatVal() < other.getFloatVal();
}

bool Number::operator<=(const Number& other) const
{
	return isInt && other.isInt ?
		intVal <= other.intVal :
		getFloatVal() <= other.getFloatVal();
}

Ref::Ref() : refCount(1) {}
ListRef::ListRef(List l) : Ref(), listVal(l) {}
MapRef::MapRef(Map m) : Ref(), mapVal(m) {}