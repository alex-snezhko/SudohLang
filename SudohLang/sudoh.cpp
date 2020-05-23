#include "sudoh.h"

var f_asd(var);
var f_asdf();

var f_asd(var _asdf)
{
	var _a = var(std::string("hello"));
	_asdf = var(1);
	f_asdf();
	f_asd(var(1));
	return null;
}

var f_asdf()
{
	var _a = var(std::string("hj"));
	return null;
}

int main()
{
	f_asd(var(1));
	f_asdf();
}
