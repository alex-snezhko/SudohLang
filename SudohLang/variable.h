#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

// to prevent implicit conversion of char* values to bool
enum class Bool { t, f };

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

struct Hash
{
    size_t operator()(const Variable& v) const
    {
        return 0;
    }
};
typedef std::vector<Variable> List;
typedef std::unordered_map<Variable, Variable, Hash/*, Cmp, std::less<const Variable&>, std::allocator<std::pair<Variable, Variable>>*/> Map;

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

bool operator<(const Variable& v1, const Variable& v2);

bool operator==(const Variable& v1, const Variable& v2);

bool operator!=(const Variable& v1, const Variable& v2);

struct Variable
{
    enum { number, boolean, str, list, mapT, null } type;

    union Val
    {
        Number numVal;
        bool boolVal;
        std::string* stringRef;
        Ref* sharedRef;

        Val(Number val) : numVal(val) {}
        Val(bool val) : boolVal(val) {}
        Val(std::string val) : stringRef(new std::string(val)) {}
        Val(Ref* val) : sharedRef(val) {}
    } val;

    Variable(const Variable& other) : type(other.type), val(other.val)
    {
        if (other.type == str)
        {
            val.stringRef = new std::string(*other.val.stringRef);
        }
    }

    void freeMem()
    {
        if (type == list || type == mapT)
        {
            if (--val.sharedRef->refCount == 0)
            {
                delete val.sharedRef;
            }
        }
        if (type == str)
        {
            delete val.stringRef;
        }
    }

    ~Variable() { freeMem(); }

    Variable() : type(null), val(false) {}
    Variable(Number n) : type(number), val(n) {}
    Variable(Bool b) : type(boolean), val(b == Bool::t) {}
    Variable(const char* s) : type(str), val(std::string(s)) {}
    Variable(List l) : type(list), val(new ListRef(l)) {}
    Variable(Map m) : type(mapT), val(new MapRef(m)) {}

    Variable operator+(const Variable& other)
    {
        switch (type)
        {
        case Variable::number:
            if (other.type != number)
            {
                throw std::exception();
            }
            //return numVal + other.numVal;
        case Variable::str:
            break;
        case Variable::list:
            break;
        default:
            throw std::exception();
            break;
        }
        return nullptr;
    }

    Variable& operator=(const Variable& other)
    {
        if (this != &other)
        {
            freeMem();
            type = other.type;
            switch (other.type)
            {
            case number:
                val.numVal = other.val.numVal;
                break;
            case boolean:
                val.boolVal = other.val.boolVal;
                break;
            case str:
                val.boolVal = other.val.boolVal;
                break;
            case list:
            case mapT:
                val.sharedRef = other.val.sharedRef;
                val.sharedRef->refCount++;
                break;
            }
        }
        return *this;
    }

    /*bool operator<(const Variable& other) const
    {
        return true;
    }*/

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

    Variable& operator[](const Variable& index)
    {
        switch (type)
        {
        case str:
        {
            if (index.type != number)
            {
                throw std::exception();
            }
            const char c[2]{ (*val.stringRef)[index.val.numVal.intVal()], '\0' };
            Variable st = c;
            return st; // Sudoh strings immutable; trying to set value of character in string won't even compile
        }
        case list:
            break;
        case mapT:
        {
            Map& m = ((MapRef*)(val.sharedRef))->mapVal;
            Variable& a = m[index];
            /*try
            {
                auto a = m.at(index);
            }
            catch (std::out_of_range&)
            {
                m.
            }*/
            return a;
        }
        default:
            throw std::exception();
        }
        return *this;
    }
};


