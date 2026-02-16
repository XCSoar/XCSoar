// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "RadioFrequency.hpp"
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
 * NOTE: returned pointers reference thread-local buffers owned by the resolver
 * call. They remain valid only until the next ResolveInfo() invocation on the
 * same thread and must not be cached or shared across threads.
 */
struct ResolvedInfo {
  const char *pilot = nullptr;
  const char *plane_type = nullptr;
  const char *registration = nullptr;
  const char *callsign = nullptr;
  const char *airfield = nullptr;  // FLARMnet-only
  RadioFrequency frequency = RadioFrequency::Null();
  ResolvedSource source = ResolvedSource::NONE;

  /**
   * Checks if any resolved information fields are available.
   */
  bool IsEmpty() const noexcept {
    return pilot == nullptr &&
           plane_type == nullptr &&
           registration == nullptr &&
           callsign == nullptr &&
           airfield == nullptr;
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
ResolveInfo(FlarmId id) noexcept;

/**
 * Retrieves the localized string corresponding to the resolved source enum.
 */
[[gnu::const]]
const char *ToString(ResolvedSource source) noexcept;

} // namespace FlarmDetails
