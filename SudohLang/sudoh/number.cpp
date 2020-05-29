#include "number.h"
#include "runtime_ex.h"
#include <cmath>

constexpr double EPSILON = 0.00000000001;

Number::Number(double f)
{
	setNew(f);
}

void Number::setNew(double f)
{
	double intPart;
	if (abs(std::modf(f, &intPart)) < EPSILON)
	{
		val = intPart;
		isInt = true;
	}
	else
	{
		val = f;
		isInt = false;
	}
}

Number Number::operator+(const Number& other) const
{
	return val + other.val;
}

Number Number::operator-(const Number& other) const
{
	return val - other.val;
}

Number Number::operator*(const Number& other) const
{
	return val * other.val;
}

Number Number::operator/(const Number& other) const
{
	if (other.val == 0.0)
	{
		runtimeException("division by zero");
	}
	return val / other.val;
}

Number Number::operator%(const Number& other) const
{
	if (other.val == 0.0)
	{
		runtimeException("division by zero");
	}
	return fmod(val, other.val);
}

void Number::operator+=(const Number& other)
{
	setNew(val + other.val);
}

void Number::operator-=(const Number& other)
{
	setNew(val - other.val);
}

void Number::operator*=(const Number& other)
{
	setNew(val * other.val);
}

void Number::operator/=(const Number& other)
{
	if (other.val == 0.0)
	{
		runtimeException("division by zero");
	}

	setNew(val / other.val);
}

void Number::operator%=(const Number& other)
{
	if (other.val == 0.0)
	{
		runtimeException("division by zero");
	}

	setNew(fmod(val, other.val));
}

bool Number::operator==(const Number& other) const
{
	return fabs(val - other.val) < EPSILON;
}

bool Number::operator!=(const Number& other) const
{
	return fabs(val - other.val) >= EPSILON;
}

bool Number::operator<(const Number& other) const
{
	return val < other.val;
}

bool Number::operator<=(const Number& other) const
{
	return val <= other.val;
}
