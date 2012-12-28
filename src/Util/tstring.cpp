#include "tstring.hpp"

#include <tchar.h>

#define WHITESPACE _T(" \r\t")

tstring &
trim_inplace(tstring &s)
{
  tstring::size_type n;

  n = s.find_first_not_of(WHITESPACE);
  if (n != tstring::npos)
    s.erase(0, n);

  n = s.find_last_not_of(WHITESPACE);
  if (n != tstring::npos)
    s.erase(n + 1, s.length());

  return s;
}
