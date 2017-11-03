#include "PCH/stdafx.h"
#include "Utils.h"

namespace GlassEngine
{
	std::vector<std::string> Split(const std::string& s, char delim)
	{
		std::vector<std::string> elems;
		Split(s, delim, std::back_inserter(elems));
		return elems;
	}
}
