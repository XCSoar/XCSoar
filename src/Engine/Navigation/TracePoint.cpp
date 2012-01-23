#include "TracePoint.hpp"
#include "Aircraft.hpp"

TracePoint::TracePoint(const AircraftState &state):
  SearchPoint(state.location),
  time((int)state.time),
  drift_factor(sigmoid(state.altitude_agl / 100) * 256),
  altitude(state.altitude),
  vario(state.netto_vario)
{
}
