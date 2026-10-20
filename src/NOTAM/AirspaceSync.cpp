// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceSync.hpp"
#include "NOTAM.hpp"
#include "Converter.hpp"
#include "Filter.hpp"
#include "Airspace/Airspaces.hpp"
#include "LogFile.hpp"

#include <vector>

namespace NOTAMAirspaceSync {

void
Rebuild(Airspaces &airspaces,
        const std::vector<struct NOTAM> &notams,
        const NOTAMSettings &settings,
        const std::chrono::system_clock::time_point now)
{
  LogFmt("NOTAM: UpdateAirspaces - converting {} NOTAMs to airspaces",
         static_cast<unsigned>(notams.size()));

  std::vector<AirspacePtr> fresh;
  unsigned added_count = 0;
  unsigned filtered_count = 0;

  try {
    for (const auto &notam : notams) {
      try {
        if (!NOTAMFilter::ShouldDisplay(notam, settings, now, false)) {
          ++filtered_count;
          continue;
        }

        auto airspace = NOTAMConverter::BuildNOTAMAirspace(notam);
        if (!airspace)
          continue;

        fresh.push_back(std::move(airspace));
        ++added_count;
      } catch (const std::exception &e) {
        LogFmt("NOTAM: Error creating airspace for NOTAM '{}': {}",
               notam.number.c_str(), e.what());
        throw;
      }
    }
  } catch (const std::exception &e) {
    LogFmt("NOTAM: Failed to rebuild airspaces: {}", e.what());
    throw;
  } catch (...) {
    LogFmt("NOTAM: Failed to rebuild airspaces");
    throw;
  }

  airspaces.EraseOrigin(AirspaceOrigin::NOTAM);
  for (auto &airspace : fresh)
    airspaces.Add(std::move(airspace));
  airspaces.Optimise();

  LogFmt("NOTAM: UpdateAirspaces completed - added {} NOTAMs, filtered {} "
         "NOTAMs",
         added_count, filtered_count);
}

} // namespace NOTAMAirspaceSync
