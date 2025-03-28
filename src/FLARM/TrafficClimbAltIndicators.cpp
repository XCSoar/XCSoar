#include "FLARM/TrafficClimbAltIndicators.hpp"
#include "Interface.hpp"

TrafficClimbAltIndicators
TrafficClimbAltIndicators::GetClimbAltIndicators(const FlarmTraffic &traffic) noexcept
{
   return GetClimbAltIndicators(traffic,
                                CommonInterface::GetComputerSettings().polar.glide_polar_task.GetMC(),
                                CommonInterface::Calculated().average);
}