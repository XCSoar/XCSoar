#ifndef TSTRING_HPP
#define TSTRING_HPP

/** \file */

#include <string>

#ifdef _UNICODE
using tstring = std::wstring;
#else
using tstring = std::string;
#endif

tstring &
trim_inplace(tstring &s);

#endif
