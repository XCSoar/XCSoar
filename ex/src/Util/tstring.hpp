#ifndef TSTRING_HPP
#define TSTRING_HPP

#include <string>

#ifdef _UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

#endif
