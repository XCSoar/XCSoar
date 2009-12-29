#ifndef OLC_RULES_HPP
#define OLC_RULES_HPP

#include "Util/tstring.hpp"

enum OLCRules {
  OLC_Sprint=0,
  OLC_FAI,
  OLC_Classic
};

tstring rule_as_text(OLCRules the_rule);

#endif
