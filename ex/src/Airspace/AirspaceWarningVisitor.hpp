#ifndef AIRSPACE_WARNING_VISITOR_HPP
#define AIRSPACE_WARNING_VISITOR_HPP

#include "AirspaceWarning.hpp"

class AirspaceWarningVisitor:
  public BaseVisitor,
  public Visitor<AirspaceWarning>
{
public:

};


#endif

