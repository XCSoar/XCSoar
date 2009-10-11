
#include "CylinderAATPoint.hpp"

GEOPOINT CylinderAATPoint::get_boundary_parametric(double t) 
{ 
  return oz.get_boundary_parametric(t);
}
