#ifndef AIRSPACE_VISITOR_HPP
#define AIRSPACE_VISITOR_HPP

#include "Airspace.hpp"
class AbstractAirspace;
class AirspacePolygon;
class AirspaceCircle;
#include "Util/GenericVisitor.hpp"

/**
 * Generic visitor for airspaces
 */
class AirspaceVisitor:
  public TreeVisitor<Airspace>,
  public Visitor<AirspacePolygon>, 
  public Visitor<AirspaceCircle>
{
private:    
};

#endif
