#include "TracePoint.hpp"

void reset_rank(TracePointVector& vec)
{
  for (TracePointVector::iterator it = vec.begin();
       it != vec.end(); ++it) {
    it->rank = 0;
  }
}
