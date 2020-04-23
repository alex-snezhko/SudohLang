#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

enum class Type { number, boolean, str, list, mapT, null };

struct Number
{
    union
    {
        int intVal;
        double floatVal;
    };
    bool isInt;

    int getIntVal() const
    {
        if (!isInt)
        {
            throw std::exception();
        }
        return intVal;
    }
    double getFloatVal() const { return isInt ? (double)intVal : floatVal; }

    Number(int i) : intVal(i), isInt(true) {}
    Number(double f) : floatVal(f), isInt(false) {}

    Number operator+(const Number& other) const
    {
        return isInt && other.isInt ?
            Number(getIntVal() + other.getIntVal()) :
            Number(getFloatVal() + other.getFloatVal());
    }

    Number operator-(const Number& other) const
    {
        return isInt && other.isInt ?
            Number(getIntVal() - other.getIntVal()) :
            Number(getFloatVal() - other.getFloatVal());
    }

    Number operator*(const Number& other) const
    {
        return isInt && other.isInt ?
            Number(getIntVal() * other.getIntVal()) :
            Number(getFloatVal() * other.getFloatVal());
    }

    Number operator/(const Number& other) const
    {
        return isInt && other.isInt && (getIntVal() % other.getIntVal() == 0) ?
            Number(getIntVal() / other.getIntVal()) :
            Number(getFloatVal() / other.getFloatVal());
    }

    bool operator==(const Number& other) const
    {
        return isInt && other.isInt ?
            getIntVal() == other.getIntVal() :
            getFloatVal() == other.getFloatVal();
    }

    bool operator!=(const Number& other) const
    {
        return !operator==(other);
    }

    bool operator<(const Number& other) const
    {

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
typedef std::string String;
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

struct Variable
{
    Type type;

    union Val
    {
        Number numVal;
        bool boolVal;
        String* stringRef;
        Ref* sharedRef;

        Val(Number val) : numVal(val) {}
        Val(bool val) : boolVal(val) {}
        Val(String val) : stringRef(new String(val)) {}
        Val(Ref* val) : sharedRef(val) {}
    } val;

    Variable() : type(Type::null), val(false) {}
    Variable(int n) : type(Type::number), val(Number(n)) {}
    Variable(double n) : type(Type::number), val(Number(n)) {}
    Variable(Number n) : type(Type::number), val(n) {}
    Variable(bool b) : type(Type::boolean), val(b) {}
    Variable(const char* s) : type(Type::str), val(String(s)) {}
    Variable(String s) : type(Type::str), val(s) {}
    Variable(List l) : type(Type::list), val(new ListRef(l)) {}
    Variable(Map m) : type(Type::mapT), val(new MapRef(m)) {}

    void setValue(const Variable& other)
    {
        switch (other.type)
        {
        case Type::number:
            val.numVal = other.val.numVal;
            break;
        case Type::boolean:
            val.boolVal = other.val.boolVal;
            break;
        case Type::str:
            val.stringRef = new String(*other.val.stringRef);
            break;
        case Type::list:
        case Type::mapT:
            val.sharedRef = other.val.sharedRef;
            val.sharedRef->refCount++;
            break;
        }
    }

    Variable(const Variable& other) : type(other.type), val(other.val) { setValue(other); }

    void freeMem()
    {
        if (type == Type::list || type == Type::mapT)
        {
            if (--val.sharedRef->refCount == 0)
            {
                delete val.sharedRef;
            }
        }
        if (type == Type::str)
        {
            delete val.stringRef;
        }
    }

    ~Variable() { freeMem(); }

    String toString() const
    {
        switch (type)
        {
        case Type::number:
            if (val.numVal.isInt)
            {
                return std::to_string(val.numVal.intVal);
            }
            return std::to_string(val.numVal.floatVal);
        case Type::boolean:
            return std::to_string(val.boolVal);
        case Type::str:
            return *val.stringRef;
        case Type::list:
            return "list";//TODO
        case Type::mapT:
            return "map";//TODO
        case Type::null:
            return "null";
        }
    }

    Variable operator+(const Variable& other) const
    {
        switch (type)
        {
        case Type::number:
            if (other.type == Type::str)
            {
                return toString() + *other.val.stringRef;
            }
            if (other.type == Type::number)
            {
                return val.numVal + other.val.numVal;
            }
            throw std::exception();
        case Type::str:
            return *val.stringRef + other.toString();
        case Type::list:
            return Variable();//TODO
        default:
            throw std::exception();
        }
    }

    Variable& operator=(const Variable& other)
    {
        if (this != &other)
        {
            freeMem();
            type = other.type;
            setValue(other);
        }
        return *this;
    }

    bool operator==(const Variable& other) const
    {
        if (other.type == Type::null)
        {
            return type == Type::null;
        }
        if (type == Type::null)
        {
            return other.type == Type::null;
        }

        if (other.type != type)
        {
            throw std::exception();
        }

        switch (other.type)
        {
        case Type::number:
            return val.numVal == other.val.numVal;
        case Type::boolean:
            return val.boolVal == other.val.boolVal;
        case Type::str:
            return *val.stringRef == *other.val.stringRef;
        case Type::list:
        case Type::mapT:
            return val.sharedRef == other.val.sharedRef;
        }
        return false;
    }

    bool operator!=(const Variable& other) const
    {
        return !operator==(other);
    }

    bool operator<(const Variable& other) const
    {
        return false;//o1.stringVal < other.stringVal;
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

    Variable& operator[](const Variable& index)
    {
        switch (type)
        {
        case Type::str:
        {
            if (index.type != Type::number)
            {
                throw std::exception();
            }
            const char c[2]{ (*val.stringRef)[index.val.numVal.getIntVal()], '\0' };
            Variable st = c;
            return st; // Sudoh strings immutable; trying to set value of character in string won't even transpile
        }
        case Type::list:
            break;
        case Type::mapT:
        {
            Map& m = ((MapRef*)(val.sharedRef))->mapVal;
            return m[index];
        }
        default:
            throw std::exception();
        }
        return *this;
    }
};
