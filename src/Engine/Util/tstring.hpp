#ifndef TSTRING_HPP
#define TSTRING_HPP

#include <string>
#include <sstream>

#include <tchar.h>

#ifdef _UNICODE
typedef std::wstring tstring;
typedef std::wostringstream tstringstream;
#else
typedef std::string tstring;
typedef std::ostringstream tstringstream;
#endif

tstring trim(const tstring& the_string);

#endif
