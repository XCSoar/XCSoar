#ifndef AIRSPACE_WARNING_VISITOR_HPP
#define AIRSPACE_WARNING_VISITOR_HPP

#include "AirspaceWarning.hpp"

/**
 * Generic visitor for AirspaceWarning system
 */
class AirspaceWarningVisitor:
  public BaseVisitor,
  public Visitor<AirspaceWarning>
{
public:

};


#endif

