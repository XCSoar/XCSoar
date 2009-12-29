#include "OLCRules.hpp"

tstring rule_as_text(OLCRules the_rule)
{
  switch(the_rule) {
  case OLC_Sprint:
    return tstring(_T("Sprint"));
  case OLC_FAI:
    return tstring(_T("Triangle (FAI)"));
  case OLC_Classic:
    return tstring(_T("Classic"));
  default:
    return tstring(_T("Unknown"));
  };
}
