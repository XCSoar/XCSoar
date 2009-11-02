#ifndef Waypoint_VISITOR_HPP
#define Waypoint_VISITOR_HPP

#include "Waypoint.hpp"
#include "Util/GenericVisitor.hpp"

class WaypointVisitor:
  public TreeVisitor<Waypoint>,
  public Visitor<Waypoint> 
{
private:    
};

#endif
