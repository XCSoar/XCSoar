#ifndef XCSOAR_FORMATTER_TEAM_CODE_HPP
#define XCSOAR_FORMATTER_TEAM_CODE_HPP

#include "Formatter/Base.hpp"

class FormatterTeamCode: public InfoBoxFormatter {
 public:
  FormatterTeamCode(const TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual const TCHAR *Render(int *color);
};


class FormatterDiffTeamBearing: public InfoBoxFormatter {
 public:
  FormatterDiffTeamBearing(const TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual const TCHAR *Render(int *color);
};

#endif
