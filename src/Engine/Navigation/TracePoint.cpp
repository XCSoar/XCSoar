#include "TracePoint.hpp"

void reset_rank(TracePointVector& vec)
{
  for (TracePointVector::iterator it = vec.begin();
       it != vec.end(); ++it) {
    it->rank = 0;
  }
}

TracePoint::TracePoint(const AIRCRAFT_STATE &state, const TaskProjection& tp):
    SearchPoint(state.Location, tp, true),
    ALTITUDE_STATE(state),
    VARIO_STATE(state),
    time(state.Time.as_int()),
    rank(0),
    drift_factor(state.thermal_drift_factor())
{

}
