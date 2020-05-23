//#include "parser.h"
//
//void TranspiledBuffer::commitLine(bool inFunction, int currStatementScope)
//{
//	// determine whether to write new line to inside of main or to global scope (for functions)
//	std::string& commitTo = inFunction ? transpiledFunctions : transpiledMain;
//
//	// correctly indent new line
//	for (int i = 0; i < currStatementScope + !inFunction; i++)
//	{
//		commitTo += "\t";
//	}
//	commitTo += uncommittedTrans + "\n";
//	uncommittedTrans = "";
//}
//
//void TranspiledBuffer::appendToBuffer(const std::string& append)
//{
//	uncommittedTrans += append;
//}
//
//const std::string& TokenIterator::nextToken()
//{
//	return tokens[tokenNum].tokenString;
//}
//
//int TokenIterator::nextTokenScope(int currStatementScope)
//{
//	int scope = currStatementScope;
//	// special case for beginning of parse when scope is set to -1
//	if (scope == -1)
//	{
//		return 0;
//	}
//	const std::string* token = &nextToken();
//	while (*token == "\n" || *token == "\t")
//	{
//		if (*token == "\n")
//		{
//			scope = 0;
//		}
//		else if (*token == "\t")
//		{
//			scope++;
//		}
//		tokenNum++;
//		token = &nextToken();
//	}
//	// count end of input as being is scope 0
//	if (*token == END)
//	{
//		return 0;
//	}
//	return scope;
//}
//
//bool Parser::parseNextLine(std::set<bool (*)()>& extraRules)
//{
//	if (nextToken() == END)
//	{
//		return false;
//	}
//
//	// first check if this line is a standalone function call or assignment statement
//	if (parseFuncCall() || parseAssignment())
//	{
//		uncommittedTrans += ";";
//		endOfLine();
//		return true;
//	}
//
//	// check if this line is a structure (loop declaration, if statement, function declaration)
//	void (*parseAfter)(int, std::set<bool (*)()>&) = nullptr; // content to parse after structure block e.g. 'else' after 'if' block
//	bool (*extraRule)() = nullptr; // extra rule for current structure e.g. allow 'break' in loop
//	if (parseStructure(extraRule, parseAfter))
//	{
//		endOfLine();
//
//		int scope = currStatementScope;
//		// if extra parsing rule specified then parse inner block with extra rule
//		if (extraRule && extraRules.count(extraRule) == 0)
//		{
//			extraRules.insert(extraRule);
//			parseBlock(extraRules);
//			extraRules.erase(extraRule);
//		}
//		else
//		{
//			parseBlock(extraRules);
//		}
//
//		// if extra parse after block specified then do it
//		if (parseAfter)
//		{
//			parseAfter(scope, extraRules);
//		}
//		return true;
//	}
//
//	// if there are any extra rules specified at this point then check them
//	for (auto& e : extraRules)
//	{
//		if (e())
//		{
//			endOfLine();
//			return true;
//		}
//	}
//
//	// valid lines will have returned from function at this point
//	throw SyntaxException("invalid line");
//}
//
//// parse a block one scope level higher than current scope
//void Parser::parseBlock(std::set<bool (*)()>& extraRules)
//{
//	uncommittedTrans = "{";
//	commitLine();
//
//	int blockScope = currStatementScope + 1;
//	currStatementScope = nextTokenScope();
//	if (currStatementScope < blockScope)
//	{
//		throw SyntaxException("empty block not allowed");
//	}
//
//	// add new scope for new variables to be placed into
//	varsInScopeN.push_back(std::set<std::string>());
//
//	// parse each line in the block
//	while (currStatementScope == blockScope && parseNextLine(extraRules))
//	{
//		currStatementScope = nextTokenScope();
//	}
//
//	// special case to add after function definition
//	if (inFunction && blockScope == 1)
//	{
//		uncommittedTrans = "\treturn null;";
//		commitLine();
//	}
//
//	// block must be exactly one scope higher than initial scope
//	if (currStatementScope > blockScope)
//	{
//		throw SyntaxException("illegal attempt to increase indentation level");
//	}
//
//	// add closing curly brace to indicate C++ end of block
//	int temp = currStatementScope;
//	currStatementScope = blockScope - 1;
//	uncommittedTrans = "}";
//	commitLine();
//	currStatementScope = temp;
//
//	// new scope for variables no longer exists; remove it
//	varsInScopeN.pop_back();
//}
//
//// return whether a variable of given name exists at current scope
//bool NameManager::varExists(const std::string& name)
//{
//	for (const std::set<std::string>& e : varsInScopeN)
//	{
//		if (e.count(name) != 0)
//		{
//			return true;
//		}
//	}
//	return false;
//}
