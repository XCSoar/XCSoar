#ifndef TSTRING_HPP
#define TSTRING_HPP

#include <string>

#ifdef _UNICODE
#include <tchar.h>
using tstring = std::wstring;
#else
using tstring = std::string;
#endif

tstring &
trim_inplace(tstring &s);

#endif
