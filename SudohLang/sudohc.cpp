#include <iostream>
#include <fstream>
#include <sstream>
#include "sudohc.h"



void analyze(std::ifstream& file)
{
	std::stringstream contentsStream;
	contentsStream << file.rdbuf();

	// add space at end to help tokenize later
	std::string contents = contentsStream.str() + " ";

	std::string transpiled;
	int tokenBrokenAt;
	tokenBrokenAt = parse(contents, transpiled);
	std::cout << transpiled;
}

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "usage: sudohc <file.sud>\n";
		return 1;
	}
	std::string name = argv[1];
	size_t index = name.find(".sud");
	if (index == std::string::npos || index != name.length() - 4)
	{
		std::cout << "invalid file type\n";
		return 1;
	}

	std::ifstream file;
	file.open("example.sud");
	if (!file)
	{
		std::cout << "invalid file\n";
		return 1;
	}

	analyze(file);

	file.close();
	return 0;
}
