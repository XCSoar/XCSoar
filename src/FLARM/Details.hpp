// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RadioFrequency.hpp"

#include <string>

class FlarmId;
struct FlarmNetRecord;
struct MessagingRecord;

enum class ResolvedSource : unsigned char {
  NONE,
  MESSAGING,
  FLARMNET,
  COUNT, // Helper to report the number of tracked fields.
};

/**
 * Resolved human-readable FLARM fields plus metadata about their origin.
 * All string fields are owned copies that remain valid for the lifetime
 * of this object.
 */
struct ResolvedInfo {
  std::string pilot;
  std::string plane_type;
  std::string registration;
  std::string callsign;
  std::string airfield;  // FLARMnet-only
  RadioFrequency frequency = RadioFrequency::Null();
  ResolvedSource source = ResolvedSource::NONE;

  /**
   * Checks if any resolved information fields are available.
   */
  bool IsEmpty() const noexcept {
    return pilot.empty() &&
           plane_type.empty() &&
           registration.empty() &&
           callsign.empty() &&
           airfield.empty();
  }
};

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
const char *
LookupCallsign(FlarmId id) noexcept;

/**
 * Looks up the callsign in the FLARM details array
 * and the FLARMnet file and returns the FLARM id
 * @param cn Callsign
 * @return The corresponding FLARM id if found, otherwise 0
 */
[[gnu::pure]]
FlarmId
LookupId(const char *cn) noexcept;

/**
 * Adds a FLARM details couple (callsign + FLARM id)
 * to the FLARM details array
 * @param id FLARM id
 * @param name Callsign
 * @return True if successfully added, False otherwise
 */
bool
AddSecondaryItem(FlarmId id, const char *name) noexcept;

/**
 * Stores a messaging record in the database and triggers periodic save.
 * @param record The messaging record to store
 */
void
StoreMessagingRecord(const MessagingRecord &record) noexcept;

unsigned
FindIdsByCallSign(const char *cn, FlarmId array[], unsigned size) noexcept;

/**
 * Resolves human-readable fields with preference: messaging -> FLARMnet.
 * Callsign uses existing FindNameById() priority (user -> messaging -> FLARMnet).
 * Airfield is FLARMnet-only.
 */
[[gnu::pure]]
ResolvedInfo
ResolveInfo(FlarmId id);

/**
 * Retrieves the localized string corresponding to the resolved source enum.
 */
[[gnu::const]]
const char *ToString(ResolvedSource source) noexcept;

} // namespace FlarmDetails
