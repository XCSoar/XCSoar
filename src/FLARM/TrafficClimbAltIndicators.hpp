#pragma once

#include "FLARM/Traffic.hpp"
#include <cstdint>

class TrafficClimbAltIndicators
{
public:
   TrafficClimbAltIndicators() noexcept
       : climb(Climb::DOWN),
         rel_alt(RelAlt::SAME)
   {
   }

   static TrafficClimbAltIndicators
   GetClimbAltIndicators(const FlarmTraffic &traffic) noexcept;

   static TrafficClimbAltIndicators
   GetClimbAltIndicators(const FlarmTraffic &traffic, const double set_mc,
                         const double current_30s_vario) noexcept
   {
      TrafficClimbAltIndicators indicators;

      const double reference_climb_rate = (set_mc > current_30s_vario) ? set_mc : current_30s_vario; // largest of the set mc or current 30s average vario.

      if (traffic.climb_rate_avg30s >= reference_climb_rate)
         indicators.climb = Climb::GOOD;
      else if (traffic.climb_rate_avg30s > 0.0)
         indicators.climb = Climb::UP;
      else
         indicators.climb = Climb::DOWN;

      if (traffic.relative_altitude > (const RoughAltitude)similar_altitude_threshold_meters)
         indicators.rel_alt = RelAlt::ABOVE;
      else if (traffic.relative_altitude > (const RoughAltitude)-similar_altitude_threshold_meters)
         indicators.rel_alt = RelAlt::SAME;
      else
         indicators.rel_alt = RelAlt::BELOW;

      return indicators;
   }

   uint8_t get_climb_indicator() const noexcept { return (uint8_t)climb; }
   uint8_t get_rel_alt_indicator() const noexcept { return (uint8_t)rel_alt; }

   enum class Climb : uint8_t
   {
      GOOD,
      UP,
      DOWN,
   };

   enum class RelAlt : uint8_t
   {
      ABOVE,
      SAME,
      BELOW,
   };

private:
   Climb climb;
   RelAlt rel_alt;

   static constexpr int similar_altitude_threshold_meters = 50;
};