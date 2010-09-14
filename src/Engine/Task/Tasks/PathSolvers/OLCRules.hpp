#ifndef OLC_RULES_HPP
#define OLC_RULES_HPP

#include <tchar.h>

enum OLCRules {
  OLC_Sprint = 0,
  OLC_FAI,
  OLC_Classic
};

const TCHAR *
rule_as_text(OLCRules the_rule);

#endif
