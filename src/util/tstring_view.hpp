#ifndef TSTRING_VIEW_HPP
#define TSTRING_VIEW_HPP

#include <string_view>

#ifdef _UNICODE
using tstring_view = std::wstring_view;
#else
using tstring_view = std::string_view;
#endif
#endif
