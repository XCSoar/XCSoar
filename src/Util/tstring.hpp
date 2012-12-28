#ifndef TSTRING_HPP
#define TSTRING_HPP

#include <string>

#include <tchar.h>

#ifdef _UNICODE
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

tstring &
trim_inplace(tstring &s);

#endif
