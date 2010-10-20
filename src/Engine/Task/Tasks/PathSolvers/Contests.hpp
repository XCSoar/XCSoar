#ifndef OLC_RULES_HPP
#define OLC_RULES_HPP

#include <tchar.h>

enum Contests {
  OLC_Sprint = 0,
  OLC_FAI,
  OLC_Classic
};

const TCHAR *
contest_as_text(Contests the_rule);

#endif
