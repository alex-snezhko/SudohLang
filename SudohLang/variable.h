#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>

/*struct Var;
typedef std::vector<Var> List;
typedef std::map<Var, Var, std::less<Var>, std::allocator<std::pair<Var, Var>>> Map;*/

/*struct VarD
{
    enum { number, boolean, str, list, mapT, null } type;

    Val* data;

    VarD() {}
    VarD(const VarD& other) {}
    VarD operator+(const VarD& other) const {}
    VarD operator-(const VarD& other) const {}
    VarD operator*(const VarD& other) const {}
    VarD operator/(const VarD& other) const {}
    VarD operator=(const VarD& other) {}
};*/


/*struct Var
{
    enum { number, boolean, str, list, mapT, null } type;

    union ASDF
    {
        Number numVal;
        bool boolVal;
        std::string stringVal;
        List listVal;
        Map mapVal;

        void doStuff() {}
    };

    Var() {}
    Var(const Var& other) {}
    Var operator+(const Var& other) const {}
    Var operator-(const Var& other) const {}
    Var operator*(const Var& other) const {}
    Var operator/(const Var& other) const {}
    Var operator=(const Var& other) {}
};*/


struct Number
{
    union
    {
        int iVal;
        double fVal;
    };
    bool isInt;

    int intVal() const
    {
        if (!isInt)
        {
            throw std::exception();
        }
        return iVal;
    }
    double floatVal() const { return isInt ? (double)iVal : fVal; }

    Number(int i) : iVal(i), isInt(true) {}
    Number(double f) : fVal(f), isInt(false) {}

    Number operator+(const Number& other) const
    {
        return isInt && other.isInt ?
            Number(intVal() + other.intVal()) :
            Number(floatVal() + other.floatVal());
    }

    Number operator-(const Number& other) const
    {
        return isInt && other.isInt ?
            Number(intVal() - other.intVal()) :
            Number(floatVal() - other.floatVal());
    }

    Number operator*(const Number& other) const
    {
        return isInt && other.isInt ?
            Number(intVal() * other.intVal()) :
            Number(floatVal() * other.floatVal());
    }

    Number operator/(const Number& other) const
    {
        return isInt && other.isInt && (intVal() % other.intVal() == 0) ?
            Number(intVal() / other.intVal()) :
            Number(floatVal() / other.floatVal());
    }

    bool operator==(const Number& other) const
    {
        return isInt && other.isInt ?
            intVal() == other.intVal() :
            floatVal() == other.floatVal();
    }

    bool operator!=(const Number& other) const
    {
        return !operator==(other);
    }
};


struct Variable;
typedef std::vector<Variable> List;
typedef std::map<Variable, Variable, std::less<Variable>, std::allocator<std::pair<Variable, Variable>>> Map;

struct Variable
{
    enum { number, boolean, str, list, mapT, ref, null } type;

    union
    {
        Number numVal;
        bool boolVal;
        std::string stringVal;
        List listVal;
        Map mapVal;
        Variable* varRef;
    };

    Variable(const Variable& other) : type(other.type)
    {
        switch (other.type)
        {
        case number:
            numVal = other.numVal;
            break;
        case boolean:
            boolVal = other.boolVal;
            break;
        case str:
            stringVal = other.stringVal;
            break;
        default:
            varRef = (Variable*)&other;
            type = ref;
            break;
        }
    }
    ~Variable() {}

    Variable(Number val) : type(number), numVal(val) {}
    Variable(bool val) : type(boolean), boolVal(val) {}
    Variable(std::string val) : type(str), stringVal(val) {}
    Variable(List val) : type(list), listVal(val) {}
    Variable(Map val) : type(mapT), mapVal(val) {}
    Variable(nullptr_t n) {}

    Variable operator+(const Variable& other)
    {
        switch (type)
        {
        case Variable::number:
            if (other.type != number)
            {
                throw std::exception();
            }
            return numVal + other.numVal;
        case Variable::str:
            break;
        case Variable::list:
            break;
        default:
            throw std::exception();
            break;
        }
    }

    Variable& operator=(const Variable& other)
    {
        return *this;
    }

    bool operator<(const Variable& other) const
    {
        return true;
    }

    bool operator>(const Variable& other) const
    {
        return true;
    }

    bool operator<=(const Variable& other) const
    {
        return true;
    }

    bool operator>=(const Variable& other) const
    {
        return true;
    }

    Variable& operator[](const Variable index)
    {
        switch (type)
        {
        case str:
            {
                if (index.type != number)
                {
                    throw std::exception();
                }
                Variable st = std::string(1, stringVal[index.numVal.intVal()]);
                return st; // Sudoh strings immutable; trying to set value of character in string won't even compile
            }
        case list:
            break;
        case mapT:
            break;
        default:
            throw std::exception();
        }
    }

    bool operator==(Variable other)
    {
        if (other.type == null)
        {
            return type == null;
        }

        if (other.type != type)
        {
            throw std::exception();
        }

        switch (type)
        {
        case number:
            //return numVal == other.numVal;
        case boolean:
            //return boolVal == other.boolVal;
        case str:
            //return stringVal == other.stringVal;
        case list:
            //return value.listVal == other.value.listVal;
        case mapT:
            //return value.mapVal == other.value.mapVal;
        default:
            break;
        }
    }

    bool operator!=(const Variable& other)
    {
        return !operator==(other);
    }

    operator Variable() const
    {
        if (type == ref)
        {
            return *varRef;
        }
        return *this;
    }
};
