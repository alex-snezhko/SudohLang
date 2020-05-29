#include "example.h"
#include "linked.h"
#include "nothing.h"

var f_asd(var _asdf)
{
	for (var _i = var(0); _i <= var(10); _i += 1)
	{
		for (_i = var(0); _i <= var(10); _i += 1)
		{
			for (_i = var(0); _i <= var(10); _i += 1)
			{
				for (_i = var(0); _i <= var(10); _i += 1)
				{
					var _a = var(1);
				}
			}
		}
	}
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
