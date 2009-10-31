#include "FlatBoundingBox.hpp"
#include "Math/FastMath.h"

unsigned 
FlatBoundingBox::distance(const FlatBoundingBox &f) const {
  long dx = std::max(0,std::min(f.bb_ll.Longitude-bb_ur.Longitude,
                                bb_ll.Longitude-f.bb_ur.Longitude));
  long dy = std::max(0,std::min(f.bb_ll.Latitude-bb_ur.Latitude,
                                bb_ll.Latitude-f.bb_ur.Latitude));
  return isqrt4(dx*dx+dy*dy);
}
