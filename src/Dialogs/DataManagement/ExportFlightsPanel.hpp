// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * Declaration for the Export Flights dialog (moved into DataManagement).
 */
void ShowExportFlightsDialog();

/** Releases the export-flight metadata cache before the Asio event loop. */
void ShutdownExportFlightsPanel() noexcept;
