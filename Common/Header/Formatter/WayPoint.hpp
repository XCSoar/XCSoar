#ifndef XCSOAR_FORMATTER_WAYPOINT_HPP
#define XCSOAR_FORMATTER_WAYPOINT_HPP

#include "Formatter/Base.hpp"

class FormatterWaypoint: public InfoBoxFormatter {
 public:
  FormatterWaypoint(const TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual const TCHAR *Render(int *color);
};

// VENTA3 / alternates
class FormatterAlternate: public InfoBoxFormatter {
 public:
  FormatterAlternate(const TCHAR *theformat):InfoBoxFormatter(theformat) {};

  virtual const TCHAR *Render(int *color);
  virtual const TCHAR *RenderTitle(int *color);
  virtual void AssignValue(int i);
};
// VENTA3 bestlanding
/*
class FormatterBestLanding: public InfoBoxFormatter {
 public:
  FormatterBestLanding(TCHAR *theformat):InfoBoxFormatter(theformat) {};
  virtual TCHAR *Render(int *color);
  virtual TCHAR *RenderTitle(int *color);
  virtual void AssignValue(int i);
};
*/

#endif
