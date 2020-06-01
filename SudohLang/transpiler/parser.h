#include "transpiled_buf.h"
#include "name_manager.h"
#include "token_iter.h"
#include <map>

// all types in Sudoh. 'any' is used for variables or function calls as their type is not static
enum class ParsedType { number, boolean, string, list, map, null, any };

// main class for parsing/transpiling a Sudoh source file. Uses a grammar to parse contents of file
class Parser
{
	TranspiledBuffer trans;
	NameManager names;
	TokenIterator tokens;

	// flag for determining whether the parser is currently inside of a procedure
	bool inProcedure;
	int currStatementScope;

	int skipToNextRelevant();
	// adds a string to the uncommitted transpiled C++ code buffer and advances tokenNum
	void appendAndAdvance(const std::string append);

	// +-------------------------------+
	// |   Grammar parsing functions   |
	// +-------------------------------+
	void endOfLine();
	void maybeMultiline();

	bool parseNextLine(std::vector<bool(Parser::*)()>& extraRules);
	void parseBlock(std::vector<bool(Parser::*)()>& extraRules);
	bool extraParseInsideLoop();
	bool extraParseInsideProcedure();
	void parseAfterIf(int scope, std::vector<bool(Parser::*)()>& extraRules);
	void parseAfterRepeat(int scope, std::vector<bool(Parser::*)()>& extraRules);
	void afterProcedure(int scope, std::vector<bool(Parser::*)()>& extraRules);
	bool parseStructure(bool (Parser::*&additionalRule)(), void (Parser::*&parseAfter)(int, std::vector<bool (Parser::*)()>&));
	bool parseAssignment();
	bool parseProcCall();
	bool parseVar(bool lvalue);
	enum struct VarParseMode { mayBeNew, mustExist, procedureParam, forVar, forEachVar };
	bool parseVarName(VarParseMode mode);

	typedef std::map<ParsedType, std::set<ParsedType>> Operations;
	void checkBinary(const std::string translatedBinOp, ParsedType& leftType,
		const Operations& allowedOps, void (Parser::*rightFunction)(ParsedType&));
	void parseTerm(ParsedType& t);
	void parseArithmetic(ParsedType& type);
	void parseComparison(ParsedType& type);
	void parseExpr(ParsedType& type);
	void parseExpr();
	void parseExpr(const std::vector<ParsedType> allowed);
	
	void parseIncludeFile();
	void parseProcedureParameter();
	void parseMapEntry();
	int parseCommaSep(void (Parser::*parseItem)(), const std::string stop, bool printComma = true);

public:
	Parser() : inProcedure(false), currStatementScope(0) {}
	void parse(const std::string& fileName, bool main);
};
