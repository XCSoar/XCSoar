#ifndef TSTRING_HPP
#define TSTRING_HPP

#include <string>

#ifdef _UNICODE
#include <tchar.h>
typedef std::wstring tstring;
#else
typedef std::string tstring;
#endif

tstring &
trim_inplace(tstring &s);

#endif
