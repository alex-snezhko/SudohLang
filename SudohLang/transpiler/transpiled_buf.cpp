#include "parser.h"

// adds uncommited transpilation buffer to a transpiled content string after entire
// line has been checked, and clears buffer
void TranspiledBuffer::commitLine(bool inFunction, int currStatementScope)
{
	// determine whether to write new line to inside of main or to global scope (for functions)
	std::string& commitTo = inFunction ? transpiledFunctions : transpiledMain;

	// correctly indent new line
	for (int i = 0; i < currStatementScope + !inFunction; i++)
	{
		commitTo += "\t";
	}
	commitTo += uncommittedTrans + "\n";
	uncommittedTrans = "";
}

void TranspiledBuffer::appendToBuffer(const std::string append)
{
	uncommittedTrans += append;
}

void TranspiledBuffer::includeFile(const std::string fileName)
{
	transpiledIncludes += "#include \"" + fileName + ".h\"\n";
}

std::string TranspiledBuffer::fullTranspiled(bool main)
{
	return transpiledIncludes + "\n" + transpiledFunctions + (main ? transpiledMain : "");
}