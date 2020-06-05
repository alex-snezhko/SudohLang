#include "parser.h"
#include <iostream>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cout << "Usage: sudohc <file.sud>\n";
		return 1;
	}
	const std::string fileName = argv[1];
	size_t index = fileName.find(".sud");
	if (index != fileName.length() - 4)
	{
		std::cout << "File extension must be '.sud'\n";
		return 1;
	}

	const std::string noExtension = fileName.substr(0, index);

	Parser p;
	p.parse(noExtension, true);
	std::cout << "Compilation successful.\n";

	return 0;
}
