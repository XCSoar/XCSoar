#include "TracePoint.hpp"


TracePoint::TracePoint(const AIRCRAFT_STATE &state, const TaskProjection& tp):
    SearchPoint(state.Location, tp, true),
    ALTITUDE_STATE(state),
    VARIO_STATE(state),
    time((int)state.Time),
    drift_factor(state.thermal_drift_factor())
{

}
