#ifndef TSTRING_HPP
#define TSTRING_HPP

#include <string>
#include <sstream>

#ifdef _UNICODE
#include <tchar.h>
typedef std::wstring tstring;
typedef std::wostringstream tstringstream;
#else
typedef std::string tstring;
typedef std::ostringstream tstringstream;
#define _T(x) (x)

#endif

#endif
