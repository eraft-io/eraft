#ifndef __TRIVIALDB_UTILS__
#define __TRIVIALDB_UTILS__

#include <cctype>
#include <cstring>
#include <regex>
#include <string>

template<typename T>
inline int basic_type_comparer(T x, T y)
{
	if(x < y) return -1;
	return x == y ? 0 : 1;
}

inline int integer_comparer(int x, int y)
{
	return basic_type_comparer(x, y);
}

inline int float_comparer(float x, float y)
{
	return basic_type_comparer(x, y);
}

inline int integer_bin_comparer(const char *x, const char *y)
{
	return integer_comparer(*(int*)x, *(int*)y);
}

inline int float_bin_comparer(const char *x, const char *y)
{
	return float_comparer(*(float*)x, *(float*)y);
}

inline int string_comparer(const char *x, const char *y)
{
	return std::strcmp(x, y);
}

inline int strcasecmp(const char *s1, const char *s2)
{
	int c1, c2;
	for(;;)
	{
		c1 = std::tolower((unsigned char)*s1++);
		c2 = std::tolower((unsigned char)*s2++);
		if(c1 == 0 || c1 != c2)
			return c1 - c2;
	}
}

inline bool strlike(const char *s1, const char *s2)
{
	// See: https://docs.microsoft.com/en-us/sql/t-sql/language-elements/like-transact-sql?view=sql-server-2017
	enum {
		STATUS_A,
		STATUS_SLASH,
	} status = STATUS_A;
	std::string pattern;

	for(int i = 0; s2[i]; ++i)
	{
		switch(status)
		{
			case STATUS_A:
				switch(s2[i])
				{
					case '\\':
						status = STATUS_SLASH;
						break;
					case '%':
						pattern += ".*";
						break;
					case '_':
						pattern.push_back('.');
						break;
					default:
						pattern.push_back(s2[i]);
						break;
				}
				break;
			case STATUS_SLASH:
				switch(s2[i])
				{
					case '%':
					case '_':
						pattern.push_back(s2[i]);
						break;
					default:
						pattern.push_back('\\');
						pattern.push_back(s2[i]);
						break;
				}
				status = STATUS_A;
				break;
		}
	}

	return std::regex_match(s1, std::regex(pattern));
}

#endif
