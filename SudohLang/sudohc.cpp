#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include <boost/spirit/include/qi.hpp>

#define NAME_RE "[a-zA-Z_]+[a-zA-Z0-9_]*"
#define TYPE_RE "(number|boolean|string|list|map)"
#define INT_RE "([0-9]+)"
#define FLOAT_RE "([0-9]*.[0-9]+)"
#define STRING_RE "(\".*\")"
#define VAL_RE ""
#define LIST_RE "([" VAL_RE "(," VAL_RE ")*])"
#define BOOL_RE "((not )? (" VAL_RE " (<=|<|is|>|>=) " VAL_RE ")|)"

#define FUNCCALL_RE NAME_RE " \\( (" NAME_RE " (, " NAME_RE ")*)? \\)"
#define IF_RE "if " BOOL_RE
#define FUNCDEF_RE "function " NAME_RE " \\( " NAME_RE "( : " TYPE_RE ")?" \
                      "( , " NAME_RE "( : " TYPE_RE ")?)* \\) (returns " TYPE_RE ")?"

using namespace std;
using namespace boost::spirit;
using namespace boost::phoenix;

const string keywords[] = {
    "for", "in", "to", "while", "do"
};

string transpiled;

struct ParseException : public exception
{
    string msg;
    ParseException(string str) : msg(str) {}
    const char *what() const throw() { return msg.c_str(); }
};

struct Variable
{
    enum Type { numInt, numFloat, boolean, str, list, mapT, function };

    union Value
    {
        int intVal;
        double floatVal;
        bool boolVal;
        string stringVal;
        vector<Value> listVal;
        map<Value, Value> mapVal;

        // Value does not dynamically allocate data; specify destructor so that
        // compiler does not complain
        ~Value() {}
    };

    Type type;
    Value value;
};

map<string, Variable> inScope;

// for now tokens must be seperated by spaces
// TODO make use of regex
vector<string> tokenize(string line)
{
    // symbols which act as both delimiters and tokens themselves
    static const char *delimTokens[] = {
        " ", ",", ":", "\"",
        "(", ")", "[", "]", "{", "}",
        "=", "+", "-", "*", "/",
        "+=", "-=", "*=", "/=",
        "<", "<=", ">", ">="
    };
    static const int NUM_DELIM = sizeof(delimTokens) / sizeof(const char *);

    vector<string> tokens;

    // treat line as if there were a delimiter at the end
    // (for algorithm to work correctly)
    line += " ";
    int idxBeginToken = 0;
    for (size_t curr = 0; curr < line.length(); curr++)
    {
        // check if the token at index is a delimiter
        const char *delim = nullptr;
        for (size_t di = 0; di < NUM_DELIM; di++)
        {
            const char *d = delimTokens[di];
            if (line.compare(curr, strlen(d), d) == 0)
            {
                delim = d;
                break;
            }
        }

        // if token at index was delimiter then tokenize it
        if (delim)
        {
            // add new token
            if (curr > idxBeginToken)
            {
                tokens.push_back(line.substr(idxBeginToken, curr - idxBeginToken));
            }
            // add delimiter as a token as well (if it is not space)
            if (*delim != ' ')
            {
                // if delimeter is a string then close the string if it is valid
                if (*delim == '\"')
                {
                    // try to find closing quote
                    int close;
                    for (close = curr + 1; close < line.length(); close++)
                    {
                        if (line[close] == '\"' && line[close - 1] != '\\')
                        {
                            break;
                        }
                    }
                    // if loop iterated through rest of string but still
                    // didnt find closing quote then string is mal-formed
                    if (close == line.length())
                    {
                        throw ParseException("malformed string");
                    }

                    tokens.push_back(line.substr(curr, close - curr + 1));
                    curr = close;
                }
                else
                {
                    tokens.push_back(delim);
                    curr += strlen(delim) - 1;
                }
            }

            idxBeginToken = curr + 1;
        }
    }

    return tokens;
}

bool validName(const string &str)
{
    return regex_match(str, regex(NAME_RE));
}

bool validType(const string &str)
{
    return true;
}

// TODO make use of regex
bool functionDef(const vector<string> &tokens)
{
    int index = 0;
    if (tokens[index++] != "function")
        return false;

    string name = tokens[index++];
    if (!validName(name))
        throw ParseException("Invalid function name.");

    Variable var = { Variable::function, { 0 } };

    vector<Variable> params;
    if (tokens[index++] == "(")
    {
        bool more = true;
        while (more)
        {
            if (index + 2 > tokens.size())
                throw ParseException("Each argument must be specified as such: '<name>: <type>'. "
                    "Parameterless functions should omit parenthesis in their signatures.");
            string varName = tokens[index++];
            if (!validName(varName))
                throw ParseException("Invalid argument name");
            if (tokens[index++] != ":")
                throw ParseException("Each argument must be specified as such: <name>: <type>.");

            string varType = tokens[index++];
            if (!validType(varType))
                throw ParseException("Invalid argument type. Valid types are number, bool, string, list, and map.");

            // TODO convert to actual type

            if (tokens[index++] != ",")
                more = false;

            //varsInScope[varName] = val
        }

        // -1 because loop will have incremented index past possible ')' before ending
        if (tokens[index - 1] != ")")
            throw ParseException("Function parameter lists should only contain a list of comma-seperated "
                "arguments and be completed with a closing parenthesis.");
    }

    if (index < tokens.size() && (tokens[index] != "returns" || ++index != tokens.size() - 1))
        throw ParseException("A function which returns data should contain "
            "'returns <type>' at the end of its signature.");

    string retType = tokens[index];
    if (!validType(retType))
        throw ParseException("Invalid return type. Valid types are number, bool, string, list, and map.");

    // TODO convert to actual return type
    return true;
}

/*struct Name : public qi::grammar<Name>
{
    qi::rule<string::const_iterator, unsigned()> start;

    Name() : Name::base_type(start)
    {
        start = qi::char_("a-zA-Z_") >> *(qi::char_("a-zA-Z0-9_"));
    }
} /*name;*/

template <typename T>
struct Lexem
{
    string accept;
    void (*onAccept)(T &);

    Lexem(string str) : accept(str), onAccept(nullptr) {}
    Lexem(string str, void (*func)(T &)) : accept(str), onAccept(func) {}
};

struct Rule
{

};

struct Grammar
{
    vector<Rule> rules;
};

void analyze(ifstream& file)
{
    string contents;
    std::getline(file, contents);
    vector<string> tokens = tokenize(contents);
    if (tokens.size() == 0)
    {
        return;
    }

    // create a string where all tokens are separated by spaces
    string spaceSep = tokens[0];
    for (auto t = tokens.begin() + 1; t != tokens.end(); t++)
    {
        spaceSep += " " + *t;
    }

    static regex funcDefRegex = regex(FUNCDEF_RE);
    static regex expressionRegex = regex();

    if (regex_match(spaceSep, funcDefRegex))
    {
        functionDef(tokens);
    }

    string funcName;
    /*auto name = qi::char_("a-zA-Z_") >> *(qi::char_("a-zA-Z0-9_"));
    auto type = qi::lit("number") | qi::lit("string") | qi::lit("boolean") | qi::lit("list") | qi::lit("map");
    auto funcDef = qi::lit("function ") >> name >> "("
        >> -(name >> ":" >> type >> *("," >> name >> ":" >> type)) >> ")"
        >> -("returns " >> type);

    auto f = qi::char_('f');

    //string s = "function asdf(p1:number,p2:string) returns map";
    string s = "function";
    auto start = s.begin();
    unsigned int result;
    bool info = qi::parse(start, s.end(), f[funcasdf], result);*/

}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        cout << "usage: sudohc <file.sud>\n";
        return 1;
    }
    string name = argv[1];
    size_t index = name.find(".sud");
    if (index == string::npos || index != name.length() - 4)
    {
        cout << "invalid file type\n";
        return 1;
    }

    ifstream file;
    file.open(argv[1]);
    if (!file)
    {
        cout << "invalid file\n";
        return 1;
    }

    transpiled +=
        "#include <iostream>\n"
        "#include <string>\n"
        "#include <vector>\n"
        "#include <map>\n"
        "struct Variable\n"
        "{\n"
        "    struct Variable\n"
        "    {\n"
        "        enum Type { numInt, numFloat, boolean, str, list, mapT, function };\n"
        "        union Value\n"
        "        {\n"
        "            int intVal;\n"
        "            double floatVal;\n"
        "            bool boolVal;\n"
        "            string stringVal;\n"
        "            vector<Value> listVal;\n"
        "            map<Value, Value> mapVal;\n"
        "            ~Value() {}\n"
        "        };\n"
        "        Type type;\n"
        "        Value val;\n"
        "    };\n"
        "};\n";


    analyze(file);

    transpiled += "}\n";
    file.close();
    return 0;
}