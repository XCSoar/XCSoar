#include "Contests.hpp"

const TCHAR *
contest_as_text(Contests the_rule)
{
  switch (the_rule) {
  case OLC_Sprint:
    return _T("Sprint");
  case OLC_FAI:
    return _T("Triangle (FAI)");
  case OLC_Classic:
    return _T("Classic");
  default:
    return _T("Unknown");
  };
}
