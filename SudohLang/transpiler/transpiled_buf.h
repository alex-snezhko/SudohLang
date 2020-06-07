#ifndef TRANSPILED_BUF_H
#define TRANSPILED_BUF_H

#include <string>

class TranspiledBuffer
{
	// string which contains all statements that are not inside of a function; to be placed into main function
	std::string transpiledMain;
	// string which contains all procedure definitions
	std::string transpiledProcedures;
	// string which contains all #includes
	std::string transpiledIncludes;

	// buffer string which will be written to one of the above transpiled strings at the end of a line
	std::string uncommittedTrans;

public:
	void commitLine(bool inProcedure, int currStatementScope);
	void appendToBuffer(const std::string append);
	void includeFile(const std::string fileName);
	std::string fullTranspiled(bool main);
};

#endif
