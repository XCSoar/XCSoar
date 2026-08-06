// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"
#include "time/BrokenDateTime.hpp"

#include <chrono>
#include <optional>

/**
 * A Logger Session that ended without XCSoar's knowledge -- a crash, an OS
 * kill, or the power being cut -- and which may therefore continue a Flight
 * that is still in progress.
 *
 */
struct CutSessionCandidate {
  /** The file to append to and to replay. */
  AllocatedPath path;

  /**
   * Time of the last valid fix: the date from the file's HFDTE header
   * combined with the time of its last valid B-record.
   */
  BrokenDateTime last_fix_utc;

  /** Number of valid B-records; the progress range for the sweep. */
  unsigned b_record_count;
};

/**
 * How long after its last fix an unsigned file may still be taken for a Cut
 * Session.
 *
 * This is only a staleness bound, whose job is to stop a long-abandoned
 * unsigned file from being picked up; the discriminator is the absence of a
 * G-record.  Fifteen minutes clears the worst realistic reboot -- the
 * unflushed tail before the cut, boot, cold GPS acquisition, and the ten
 * seconds of movement before flight detection latches.
 */
static constexpr std::chrono::minutes CUT_SESSION_MAX_AGE{15};

/**
 * Look for a Cut Session among the IGC files in a directory.
 *
 * A candidate must have no G-record, at least one valid B-record, and a last
 * fix no more than \a max_age before \a reference_utc.  Where several
 * candidates qualify, the one with the latest last fix wins.
 *
 * \a reference_utc must come from the current GPS fix, never from the system
 * clock: XCSoar does not set the system clock from GPS on Linux or Android,
 * so after a hard reboot the system clock is exactly what cannot be trusted.
 * Both sides of the comparison are therefore GPS time.
 *
 * Does not throw; an unreadable directory or file yields no candidate.
 */
std::optional<CutSessionCandidate>
FindCutSession(Path directory, BrokenDateTime reference_utc,
               std::chrono::system_clock::duration max_age =
                 CUT_SESSION_MAX_AGE) noexcept;
