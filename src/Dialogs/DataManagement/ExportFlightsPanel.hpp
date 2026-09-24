// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

/** Export lists IGC and NMEA logs. WeGlide lists IGC files for upload. */
enum class ExportFlightsMode {
  EXPORT,
  WEGLIDE,
};

/** Result of the Replay file list. FILE copies the chosen path. */
enum class ReplayFlightChoice {
  CANCEL,
  DEMO,
  FILE,
};

/**
 * Declaration for the Export Flights dialog (moved into DataManagement).
 * WeGlide mode is the upload list: IGC files, Upload, and Cancel.
 */
void ShowExportFlightsDialog(ExportFlightsMode mode =
                            ExportFlightsMode::EXPORT);

/**
 * Replay file selector. Lists IGC and NMEA logs. IGC rows gain
 * takeoff, landing and duration as the scan finishes.
 * FILE replaces @p path with the chosen log.
 */
ReplayFlightChoice
PickReplayFlight(const char *caption, AllocatedPath &path);

/** Releases the export-flight metadata cache before the Asio event loop. */
void ShutdownExportFlightsPanel() noexcept;
