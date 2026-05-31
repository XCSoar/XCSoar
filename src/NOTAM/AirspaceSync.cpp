// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceSync.hpp"
#include "NOTAM.hpp"
#include "Converter.hpp"
#include "Filter.hpp"
#include "Airspace/Airspaces.hpp"
#include "LogFile.hpp"

namespace NOTAMAirspaceSync {

Result
Rebuild(Airspaces &airspaces,
        const std::vector<struct NOTAM> &notams,
        const NOTAMSettings &settings,
        const std::chrono::system_clock::time_point now,
        const std::unordered_set<const AbstractAirspace *> &previous_injected)
{
  LogFmt("NOTAM: UpdateAirspaces - converting {} NOTAMs to airspaces",
         static_cast<unsigned>(notams.size()));

  std::vector<AirspacePtr> saved_airspaces;
  for (const auto &airspace : airspaces.QueryAll()) {
    const auto &current = airspace.GetAirspace();
    if (!previous_injected.contains(&current))
      saved_airspaces.push_back(airspace.GetAirspacePtr());
  }

  LogFmt("NOTAM: Saved {} non-injected airspaces",
         static_cast<unsigned>(saved_airspaces.size()));

  Airspaces temp_airspaces;
  Result result;

  try {
    for (const auto &airspace : saved_airspaces)
      temp_airspaces.Add(airspace);

    LogFmt("NOTAM: Restored {} non-injected airspaces",
           static_cast<unsigned>(saved_airspaces.size()));

    for (const auto &notam : notams) {
      try {
        if (!NOTAMFilter::ShouldDisplay(notam, settings, now, false)) {
          ++result.filtered_count;
          continue;
        }

        auto airspace = NOTAMConverter::BuildNOTAMAirspace(notam);
        if (!airspace)
          continue;

        auto *const ptr = airspace.get();
        temp_airspaces.Add(std::move(airspace));
        result.injected.insert(ptr);
        ++result.added_count;
      } catch (const std::exception &e) {
        LogFmt("NOTAM: Error creating airspace for NOTAM '{}': {}",
               notam.number.c_str(), e.what());
        throw;
      }
    }

    temp_airspaces.Optimise();
  } catch (const std::exception &e) {
    LogFmt("NOTAM: Failed to rebuild airspaces: {}", e.what());
    throw;
  } catch (...) {
    LogFmt("NOTAM: Failed to rebuild airspaces");
    throw;
  }

  airspaces.Swap(temp_airspaces);

  LogFmt("NOTAM: UpdateAirspaces completed - added {} NOTAMs, filtered {} "
         "NOTAMs",
         result.added_count, result.filtered_count);

  return result;
}

} // namespace NOTAMAirspaceSync
