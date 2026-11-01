// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

/** Export lists IGC and NMEA logs. Upload lists IGC files. */
enum class ExportFlightsMode {
  EXPORT,
  UPLOAD,
};

/** Result of the Replay file list. FILE copies the chosen path. */
enum class ReplayFlightChoice {
  CANCEL,
  DEMO,
  FILE,
};

/**
 * Declaration for the Export Flights dialog (moved into DataManagement).
 */
void ShowExportFlightsDialog(ExportFlightsMode mode =
                            ExportFlightsMode::EXPORT);

/** Pick the service when both are on, then open the IGC list. */
void
ShowUploadFlightList() noexcept;

void
UploadSoaringScoringIgc(Path path) noexcept;

/**
 * Replay file selector. Lists IGC and NMEA logs. IGC rows gain
 * takeoff, landing and duration as the scan finishes.
 * FILE replaces @p path with the chosen log.
 */
ReplayFlightChoice
PickReplayFlight(const char *caption, AllocatedPath &path);

/** Releases the export-flight metadata cache before the Asio event loop. */
void ShutdownExportFlightsPanel() noexcept;
