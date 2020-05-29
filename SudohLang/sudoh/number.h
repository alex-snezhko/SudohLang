#pragma once

struct Number
{
	double val;
	bool isInt;

	void setNew(double val);
public:
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