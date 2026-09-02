// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "co/Task.hxx"

class CurlGlobal;
class Path;
class ProgressListener;
class RecordedFlightList;

/**
 * A client for the FLARM Hub REST API, which is the only way to read
 * the flight log of devices that do not implement the FLARM binary
 * protocol (e.g. the PowerFLARM Flex).
 */
namespace FlarmHub {

/**
 * Check whether the given host serves the FLARM Hub REST API and read
 * the list of recorded flights.
 *
 * Throws on error.
 *
 * @return false if this host has no Hub REST API; the caller shall
 * then fall back to the FLARM binary protocol
 */
Co::Task<bool>
CoReadFlightList(CurlGlobal &curl, const char *host,
                 RecordedFlightList &flight_list,
                 ProgressListener &progress);

/**
 * Download one IGC file.
 *
 * Throws on error.
 *
 * @param index the flight index from RecordedFlightInfo::flarm_hub
 */
Co::Task<void>
CoDownloadFlight(CurlGlobal &curl, const char *host, unsigned index,
                 Path path, ProgressListener &progress);

} // namespace FlarmHub
