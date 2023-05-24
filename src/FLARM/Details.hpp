// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <tchar.h>

class FlarmId;
struct FlarmNetRecord;

namespace FlarmDetails {

/**
 * Looks up the FLARM id in the FLARMNet Database
 * and returns the FLARMNet Record
 * @param id FLARM id
 * @return The corresponding FLARMNet Record if found, otherwise NULL
 */
[[gnu::pure]]
const FlarmNetRecord *
LookupRecord(FlarmId id) noexcept;

/**
 * Looks up the FLARM id in the FLARM details array
 * and the FLARMnet file and returns the callsign
 * @param id FLARM id
 * @return The corresponding callsign if found, otherwise NULL
 */
[[gnu::pure]]
const TCHAR *
LookupCallsign(FlarmId id) noexcept;

/**
 * Looks up the callsign in the FLARM details array
 * and the FLARMnet file and returns the FLARM id
 * @param cn Callsign
 * @return The corresponding FLARM id if found, otherwise 0
 */
[[gnu::pure]]
FlarmId
LookupId(const TCHAR *cn) noexcept;

/**
 * Adds a FLARM details couple (callsign + FLARM id)
 * to the FLARM details array
 * @param id FLARM id
 * @param name Callsign
 * @return True if successfully added, False otherwise
 */
bool
AddSecondaryItem(FlarmId id, const TCHAR *name) noexcept;

unsigned
FindIdsByCallSign(const TCHAR *cn, FlarmId array[], unsigned size) noexcept;

} // namespace FlarmDetails
