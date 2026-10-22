// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LayerStatus.hpp"

#ifdef HAVE_HTTP

#include "ForecastFormatter.hpp"
#include "Layers.hpp"
#include "SkySightClient.hpp"
#include "Formatter/LocalTimeFormatter.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Language/Language.hpp"

#include <ctime>

StaticString<256>
FormatSelectedLayerStatus(const SkySightClient &client,
                          const SkySight::Layer &layer) noexcept
{
  StaticString<256> second_row;

  if (layer.ShouldShowUpdating()) {
    if (client.IsThrottled())
      second_row.Format(_("Download limit reached; retrying in %u seconds..."),
                        unsigned(client.GetThrottleRemainingSeconds()));
    else if (const auto retry = client.GetDatafilesRetryRemainingSeconds();
             retry > 0 && layer.HasPendingForecastMetadata())
      second_row.Format(_("Connection failed; retrying in %u seconds..."),
                        unsigned(retry));
    else if (!layer.SupportsLiveTiles() &&
             layer.HasPendingForecastMetadata())
      second_row = _("Loading forecast steps...");
    else if (!layer.SupportsLiveTiles() && layer.decoding)
      second_row = _("Decoding forecast data...");
    else if (!layer.SupportsLiveTiles() && layer.pending_downloads > 1)
      second_row.Format(_("Preloading %u forecast steps..."),
                        layer.pending_downloads);
    else if (!layer.SupportsLiveTiles())
      second_row = _("Downloading forecast data...");
    else
      second_row = _("Updating...");
  } else if (layer.SupportsLiveTiles()) {
    if (layer.last_update != 0)
      second_row.Format(_("Live layer. Last update %s"),
                        FormatLocalTimeHHMM(
                          TimeStamp(std::chrono::duration<double>(layer.last_update)),
                          client.GetForecastDisplayOffset(layer.last_update)).c_str());
    else
      second_row = _("Live tile layer.");
  } else if (layer.forecast_datafiles.empty()) {
    second_row = _("No forecast steps available yet.");
  } else if (layer.mtime == 0) {
    if (!client.IsForecastDecodeAvailable())
      second_row = _("Forecast steps available, but this build has no NetCDF decode support.");
    else
      second_row = _("Forecast steps available. Activate or choose Time to download one.");
  } else {
    const auto now = std::time(nullptr);
    const auto age = std::chrono::seconds(now > layer.mtime
                                          ? now - layer.mtime
                                          : 0);
    if (layer.forecast_time != 0) {
      const auto step_offset =
        client.GetForecastDisplayOffset(layer.forecast_time);
      second_row.Format(_("Step %s. Data from %s to %s. Updated %s ago"),
                        SkySight::FormatForecastTimeLabel(
                          layer, layer.forecast_time,
                          step_offset).c_str(),
                        FormatLocalTimeHHMM(
                          TimeStamp(std::chrono::duration<double>(layer.from)),
                          client.GetForecastDisplayOffset(layer.from)).c_str(),
                        FormatLocalTimeHHMM(
                          TimeStamp(std::chrono::duration<double>(layer.to)),
                          client.GetForecastDisplayOffset(layer.to)).c_str(),
                        FormatTimespanSmart(age).c_str());
    } else {
      second_row.Format(_("Data from %s to %s. Updated %s ago"),
                        FormatLocalTimeHHMM(
                          TimeStamp(std::chrono::duration<double>(layer.from)),
                          client.GetForecastDisplayOffset(layer.from)).c_str(),
                        FormatLocalTimeHHMM(
                          TimeStamp(std::chrono::duration<double>(layer.to)),
                          client.GetForecastDisplayOffset(layer.to)).c_str(),
                        FormatTimespanSmart(age).c_str());
    }
  }

  return second_row;
}

#endif
