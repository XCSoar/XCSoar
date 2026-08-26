// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Renderer/WaypointRendererSettings.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Profile/Profile.hpp"
#include "Profile/Map.hpp"
#include "Profile/Current.hpp"
#include "TestUtil.hpp"
#include "util/StringFormat.hpp"

static bool
EveryTypeDisplayed(const WaypointRendererSettings &settings) noexcept
{
  for (unsigned i = 0; i < unsigned(Waypoint::Type::COUNT); ++i)
    if (!settings.IsTypeDisplayed(Waypoint::Type(i)))
      return false;
  return true;
}

int
main()
{
  plan_tests(8);

  Profile::map.Clear();

  WaypointRendererSettings settings;
  settings.SetDefaults();
  settings.LoadFromProfile();
  ok1(EveryTypeDisplayed(settings));
  ok1(settings.IsTypeDisplayed(
        Waypoint::Type(unsigned(Waypoint::Type::COUNT))));

  settings.SaveTypeDisplay(Waypoint::Type(unsigned(Waypoint::Type::COUNT)),
                           false);
  char past_count[40];
  StringFormat(past_count, sizeof(past_count), "WaypointTypeDisplay%u",
               unsigned(Waypoint::Type::COUNT));
  ok1(!Profile::Exists(past_count));
  ok1(EveryTypeDisplayed(settings));

  settings.SaveTypeDisplay(Waypoint::Type::AIRFIELD, false);
  ok1(!settings.IsTypeDisplayed(Waypoint::Type::AIRFIELD));
  ok1(settings.IsTypeDisplayed(Waypoint::Type::NORMAL));

  WaypointRendererSettings loaded;
  loaded.SetDefaults();
  loaded.LoadFromProfile();
  ok1(!loaded.IsTypeDisplayed(Waypoint::Type::AIRFIELD));
  ok1(loaded.IsTypeDisplayed(Waypoint::Type::OUTLANDING));

  return exit_status();
}
