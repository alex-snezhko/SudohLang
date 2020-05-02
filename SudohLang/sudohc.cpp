#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <regex>
#include "parser.h"
//#include <boost/spirit/include/qi.hpp>
//#include <boost/phoenix/stl/container/container.hpp>

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
//using namespace boost::spirit;
//using namespace boost::phoenix;

string transpiled;

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

    //Variable var = { Variable::function, { 0 } };

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

/*struct Expression : public qi::grammar<string::const_iterator, int()>
{
    qi::rule<string::const_iterator, int()> start, lit, term, factor, plus, minus;// , multiply, divide,
        //paren, name, negative, positive;

    Expression() : Expression::base_type(start)
    {
        //start = factor >> "+" >> factor | factor;
        //factor = "(" >> start >> ")" | lit;
        //lit = qi::char_;
        start = plus | minus | lit;
        plus = start >> "+" >> start;
        minus = start >> "-" >> start;
        lit = qi::char_;
    }
};

struct ArithmeticGrammar4 : public qi::grammar<std::string::const_iterator, int()>
{
    ArithmeticGrammar4() : ArithmeticGrammar4::base_type(start)
    {
        start = product >> *('+' >> product);
        product = factor >> *('*' >> factor);
        factor = qi::char_ | group;
        group = '(' >> start >> ')';
    }

    // as before, mirrors the template arguments of qi::grammar.
    qi::rule<string::const_iterator, int()> start, group, product, factor;
};*/

/*struct CompoundRule;

struct Lexem
{
    bool isTerminal;
    Lexem(bool terminal) : isTerminal(terminal) {}
    virtual bool acceptAtIndex(const string& str, int& idx) = 0;
};

struct Nonterminal : public Lexem
{
    CompoundRule* nonTerminalRule;

    Nonterminal(CompoundRule* rule) : Lexem(false), nonTerminalRule(rule) {}

    bool acceptAtIndex(const string& str, int& idx) override
    {
        return false;
    }
};

struct Terminal : public Lexem
{
    const regex matchRegex;

    Terminal(const char* re) : Lexem(true), matchRegex(re) {}

    // character at index should be any character contained in 'anyCharOf' array
    bool acceptAtIndex(const string& str, int& idx) override
    {
        smatch match;
        regex_search(str.begin() + idx, str.end(), match, matchRegex);
        
        int maxLen = 0;
        for (const ssub_match& a : match)
        {
            int len = a.length();
            if (len > maxLen)
            {
                maxLen = len;
            }
        }

        idx += maxLen;

        return maxLen == 0;
    }
};

struct CompoundRule
{
    vector<vector<Lexem>> toread;

    CompoundRule(vector<vector<Lexem>> read) : toread(read) {}

    vector<vector<Lexem>> acceptsSubstring(const string& str, int index)
    {
        vector<vector<Lexem>> accepts;
        for (vector<Lexem>& vec : toread)
        {
            for (Lexem& lex : vec)
            {
                int idx = index;
                if (lex.acceptAtIndex(str, idx))
                {
                    accepts.push_back(vec);
                }
            }
        }
        return accepts;
    }
};

struct Grammar
{
    vector<CompoundRule> rules;

    Grammar(vector<CompoundRule> r) : rules(r) {}
    Grammar(const Grammar& other) : rules(other.rules) {}

    bool recurse(const string& s, int idx, vector<string>& names)
    {
        char c = s[idx];
        for (CompoundRule& rule : rules)
        {
            vector<vector<Lexem>> applicableRules = rule.acceptsSubstring(s, idx);

        }
        // find rules which may be relevant here

    }

    // returns list of names used
    vector<string> match(const string& s)
    {
        vector<string> names;
        if (recurse(s, 0, names))
        {
            return names;
        }
        else
        {
            throw ParseException("invalid line");
        }
    }
    
};

vector<string> parse(const string& input, int start, Grammar grammar)
{
    return grammar.match(input);
}*/

void analyze(ifstream& file)
{
    string contents;
    std::getline(file, contents);

    /*CompoundRule rule1({ nullptr, string("+"), nullptr });
    CompoundRule rule2({ string("1") });
    CompoundRule rule3({ string("2") });
    CompoundRule rule4({ string("3") });
    CompoundRule rule5({ string("4") });
    Grammar add({ rule1, rule2, rule3, rule4, rule5 });
    parse("1+2+3+4", 0, add);*/
    /*vector<string> tokens = tokenize(contents);
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
    }*/

    string n;
    string funcName;
    //auto name = (qi::char_("a-zA-Z_") >> *(qi::char_("a-zA-Z0-9_")));
    /*auto type = qi::lit("number") | qi::lit("string") | qi::lit("boolean") | qi::lit("list") | qi::lit("map");
    auto funcDef = qi::lit("function ") >> name >> "("
        >> -(name >> ":" >> type >> *("," >> name >> ":" >> type)) >> ")"
        >> -("returns " >> type);*/
    //using boost::phoenix::ref;
    //using boost::phoenix::push_back;

    //string s = "function asdf(p1:number,p2:string) returns map";
    string s = "a+b";
    auto start = s.begin();
    vector<string> result;
    int resulti;
    //bool info = qi::parse(start, s.end(), Expression(), resulti);
    //bool info = qi::parse(start, s.end(), Expression(), resulti);
    //bool r = parse({"a", "<-", "1", "+", "2"});
    bool r = parse({
        "t", "<-", "false", "\n",
        "if", "not", "t", "then", "\n",
        "\t", "m", "<-", "{", "\"head\"", "<-", "null", ",", "\"size\"", "<-", "0", "}", "\n",
        "else", "\n",
        "\t", "a", "<-", "3"}
    );
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
