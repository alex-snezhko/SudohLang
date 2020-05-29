#include "runtime_ex.h"
#include <iostream>

void runtimeException(const std::string msg)
{
	std::string output = "Runtime exception: " + msg + "; terminating program";
	std::cout << "\n+";
	for (int i = 0; i < output.length() + 4; i++)
	{
		std::cout << "-";
	}
	std::cout << "+\n|  " << output << "  |\n+";
	for (int i = 0; i < output.length() + 4; i++)
	{
		std::cout << "-";
	}
	std::cout << "+\n";
	exit(0);
}
