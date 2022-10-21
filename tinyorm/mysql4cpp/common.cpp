#include "common.h"
#include <ctype.h>
#include <algorithm>

std::string Util::strip(const std::string& str)
{
	int i = 0;
	int j = str.size() - 1;
	if (j < i)
		return std::string();
	while (i < j && isspace(str.at(i)))
		i++;
	if (i == j)
		return std::string();
	while (j > i && isspace(str.at(j)))
		j--;
	return str.substr(i, j - i + 1);
}

std::vector<std::string> Util::split(const std::string& str, const std::string& seps)
{
	std::vector<std::string> arr;
	size_t l = 0; size_t r = 0;
	while (l < str.size())
	{
		r = str.find_first_of(seps, l);
		if (r == l)
		{
			l++;
			continue;
		}
		if (r == std::string::npos)
		{
			arr.push_back(str.substr(l));
			break;
		}
		arr.push_back(str.substr(l, r - l));
		l = r + 1;
	}
	return arr;
}

void Util::toLowerCase(std::string& str)
{
	std::transform(str.cbegin(), str.cend(), str.begin(), ::tolower);
}

void Util::toUpperCase(std::string& str)
{
	std::transform(str.cbegin(), str.cend(), str.begin(), ::toupper);
}
