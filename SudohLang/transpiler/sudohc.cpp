#include "sudohc.h"
#include <iostream>
#include <fstream>
#include <sstream>

int main(int argc, char** argv)
{
	/*if (argc != 2)
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
	}*/

	std::ifstream file;
	file.open("example.sud");
	if (!file)
	{
		std::cout << "invalid file\n";
		return 1;
	}

	std::stringstream contentsStream;
	contentsStream << file.rdbuf();

	// add space at end to help tokenize later
	std::string contents = contentsStream.str() + " ";
	file.close();

	std::string transpiled;
	if (parse(contents, transpiled))
	{
		std::cout << transpiled;
		std::ofstream out;
		out.open("sudoh.cpp");
		out << transpiled;
		out.close();
	}

	return 0;
}
