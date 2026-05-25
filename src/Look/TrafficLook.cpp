// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TrafficLook.hpp"
#include "Screen/Layout.hpp"
#include "Resources.hpp"
#include "FLARM/Traffic.hpp"
#include "util/Macros.hpp"

constexpr Color TrafficLook::team_color_green;
constexpr Color TrafficLook::team_color_magenta;
constexpr Color TrafficLook::team_color_blue;
constexpr Color TrafficLook::team_color_yellow;

void
TrafficLook::Initialise(const Font &_font)
{
  safe_above_brush.Create(safe_above_color);
  safe_below_brush.Create(safe_below_color);
  warning_brush.Create(warning_color);
  warning_in_altitude_range_brush.Create(warning_in_altitude_range_color);
  alarm_brush.Create(alarm_color);

  fading_pen.Create(Pen::Style::DASH1, Layout::ScalePenWidth(1), fading_outline_color);

#ifdef ENABLE_OPENGL
  fading_brush.Create(fading_fill_color);
#endif

  unsigned width = Layout::ScalePenWidth(2);
  team_pen_green.Create(width, team_color_green);
  team_pen_blue.Create(width, team_color_blue);
  team_pen_yellow.Create(width, team_color_yellow);
  team_pen_magenta.Create(width, team_color_magenta);

  teammate_icon.LoadResource(IDB_TEAMMATE_POS_ALL);

  if (aircraft_type_icons) {
    aircraft_type_icon[0].LoadResource(IDB_TRAFFIC_UNKNOWN,
                                       IDB_TRAFFIC_UNKNOWN_HD,
                                       IDB_TRAFFIC_UNKNOWN_UHD);
    aircraft_type_icon[1].LoadResource(IDB_TRAFFIC_GLIDER,
                                       IDB_TRAFFIC_GLIDER_HD,
                                       IDB_TRAFFIC_GLIDER_UHD);
    aircraft_type_icon[2].LoadResource(IDB_TRAFFIC_TOW_PLANE,
                                       IDB_TRAFFIC_TOW_PLANE_HD,
                                       IDB_TRAFFIC_TOW_PLANE_UHD);
    aircraft_type_icon[3].LoadResource(IDB_TRAFFIC_HELICOPTER,
                                       IDB_TRAFFIC_HELICOPTER_HD,
                                       IDB_TRAFFIC_HELICOPTER_UHD);
    aircraft_type_icon[4].LoadResource(IDB_TRAFFIC_PARACHUTE,
                                       IDB_TRAFFIC_PARACHUTE_HD,
                                       IDB_TRAFFIC_PARACHUTE_UHD);
    aircraft_type_icon[5].LoadResource(IDB_TRAFFIC_DROP_PLANE,
                                       IDB_TRAFFIC_DROP_PLANE_HD,
                                       IDB_TRAFFIC_DROP_PLANE_UHD);
    aircraft_type_icon[6].LoadResource(IDB_TRAFFIC_HANG_GLIDER,
                                       IDB_TRAFFIC_HANG_GLIDER_HD,
                                       IDB_TRAFFIC_HANG_GLIDER_UHD);
    aircraft_type_icon[7].LoadResource(IDB_TRAFFIC_PARA_GLIDER,
                                       IDB_TRAFFIC_PARA_GLIDER_HD,
                                       IDB_TRAFFIC_PARA_GLIDER_UHD);
    aircraft_type_icon[8].LoadResource(IDB_TRAFFIC_POWERED_AIRCRAFT,
                                       IDB_TRAFFIC_POWERED_AIRCRAFT_HD,
                                       IDB_TRAFFIC_POWERED_AIRCRAFT_UHD);
    aircraft_type_icon[9].LoadResource(IDB_TRAFFIC_JET,
                                       IDB_TRAFFIC_JET_HD,
                                       IDB_TRAFFIC_JET_UHD);
    aircraft_type_icon[10].LoadResource(IDB_TRAFFIC_RESERVED,
                                        IDB_TRAFFIC_RESERVED_HD,
                                        IDB_TRAFFIC_RESERVED_UHD);
    aircraft_type_icon[11].LoadResource(IDB_TRAFFIC_BALLOON,
                                        IDB_TRAFFIC_BALLOON_HD,
                                        IDB_TRAFFIC_BALLOON_UHD);
    aircraft_type_icon[12].LoadResource(IDB_TRAFFIC_AIRSHIP,
                                        IDB_TRAFFIC_AIRSHIP_HD,
                                        IDB_TRAFFIC_AIRSHIP_UHD);
    aircraft_type_icon[13].LoadResource(IDB_TRAFFIC_UAV,
                                        IDB_TRAFFIC_UAV_HD,
                                        IDB_TRAFFIC_UAV_UHD);
    aircraft_type_icon[14].LoadResource(IDB_TRAFFIC_RESERVED,
                                        IDB_TRAFFIC_RESERVED_HD,
                                        IDB_TRAFFIC_RESERVED_UHD);
    aircraft_type_icon[15].LoadResource(IDB_TRAFFIC_STATIC_OBJECT,
                                          IDB_TRAFFIC_STATIC_OBJECT_HD,
                                          IDB_TRAFFIC_STATIC_OBJECT_UHD);
  }

  font = &_font;
}

const MaskedIcon &
TrafficLook::GetAircraftTypeIcon(FlarmTraffic::AircraftType type) const noexcept
{
  const unsigned index = unsigned(type);
  if (index >= ARRAY_SIZE(aircraft_type_icon))
    return aircraft_type_icon[0];

  return aircraft_type_icon[index];
}

Color
TrafficLook::GetTrafficDisplayColor(const FlarmTraffic &traffic) const noexcept
{
  switch (traffic.alarm_level) {
  case FlarmTraffic::AlarmType::LOW:
  case FlarmTraffic::AlarmType::INFO_ALERT:
    return warning_color;

  case FlarmTraffic::AlarmType::IMPORTANT:
  case FlarmTraffic::AlarmType::URGENT:
    return alarm_color;

  case FlarmTraffic::AlarmType::NONE:
    break;
  }

  if (traffic.relative_altitude > (const RoughAltitude)50)
    return safe_above_color;
  if (traffic.relative_altitude > (const RoughAltitude)-50)
    return warning_in_altitude_range_color;

  return safe_below_color;
}
