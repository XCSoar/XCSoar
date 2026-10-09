// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Id.hpp"
#include "time/Stamp.hpp"
#include "time/Validity.hpp"
#include "util/StaticArray.hxx"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

class Angle;
class BufferedOutputStream;
class NLineReader;
struct NMEAInfo;

/**
 * An estimate of how far a FLARM receives other aircraft in each
 * direction, made by XCSoar from the traffic the FLARM reports
 * ($PFLAA).  For a Classic FLARM, which has no statistics of its own
 * (the PowerFLARM's CARP, $PFLAN).
 *
 * Each contact with a FLARM target counts once in every sector it
 * passed, with the largest distance it was received at there, by its
 * bearing relative to the own ground track.  Counting every position
 * report instead would let an aircraft that stays close for a long
 * time, in the same thermal for example, outweigh all others.
 */
struct FlarmRangeEstimate {
  using TimePoint = std::chrono::system_clock::time_point;

  /** like CARP: 18 degree sectors, clockwise from the front */
  static constexpr unsigned SECTORS = 20;

  /** histogram resolution [m] */
  static constexpr unsigned BIN_WIDTH = 250;

  /**
   * Up to 26 km; the last bin also takes anything beyond.  A Classic
   * FLARM reports nothing beyond 25.5 km (its largest RANGE setting).
   */
  static constexpr unsigned BINS = 104;

  struct Sector {
    /** the number of contacts */
    uint32_t count = 0;

    /** the largest distance received [m] */
    uint32_t maximum = 0;

    /** the largest distance of each contact */
    std::array<uint32_t, BINS> histogram{};

    void Add(unsigned distance) noexcept;

    /**
     * The distance below which the fraction @p p of the contacts
     * stayed, rounded up to the histogram resolution [m].
     */
    [[gnu::pure]]
    std::optional<unsigned> Percentile(double p) const noexcept;

    /**
     * The mean of the larger half of the contacts' distances [m].
     * The smaller half is left out: an aircraft that never got far
     * away says nothing about the range.
     */
    [[gnu::pure]]
    std::optional<unsigned> UpperHalfMean() const noexcept;
  };

  std::array<Sector, SECTORS> sectors;

  /** the first and last contact counted */
  std::optional<TimePoint> first, last;

  /**
   * The sector of a bearing relative to the direction of flight.
   */
  [[gnu::const]]
  static unsigned SectorIndex(Angle relative_bearing) noexcept;

  void Clear() noexcept {
    *this = {};
  }

  [[gnu::pure]]
  uint32_t GetCount() const noexcept;

  void Add(unsigned sector, unsigned distance,
           std::optional<TimePoint> time) noexcept;

  void Add(Angle relative_bearing, unsigned distance,
           std::optional<TimePoint> time) noexcept;
};

/**
 * Feeds the FLARM traffic of each update into a #FlarmRangeEstimate.
 * A contact ends when a target has not been reported for
 * #CONTACT_TIMEOUT; its largest distance per sector is counted then.
 */
class FlarmRangeEstimator {
  /**
   * Longer than the traffic list keeps a target, so that a target
   * dropping out for a few seconds stays one contact.
   */
  static constexpr std::chrono::seconds CONTACT_TIMEOUT{30};

  struct Contact {
    FlarmId id;

    /** the last report seen */
    Validity valid;

    /** when the target was last reported */
    TimeStamp last_clock;

    /** when the last report was counted */
    std::optional<FlarmRangeEstimate::TimePoint> time;

    /** the largest distance per sector [m], 0 if none */
    std::array<uint32_t, FlarmRangeEstimate::SECTORS> maxima{};
  };

  StaticArray<Contact, 64> contacts;

  static void Count(FlarmRangeEstimate &estimate,
                    const Contact &contact) noexcept;

public:
  /**
   * @param flying only reports received in flight are counted
   */
  void Process(FlarmRangeEstimate &estimate, const NMEAInfo &basic,
               bool flying) noexcept;

  /**
   * Count all contacts that are still open, e.g. after landing.
   */
  void Flush(FlarmRangeEstimate &estimate) noexcept;
};

/**
 * Write @p estimate as text.
 */
void
SaveFlarmRangeEstimate(const FlarmRangeEstimate &estimate,
                       BufferedOutputStream &os);

/**
 * Read what SaveFlarmRangeEstimate() wrote.  Lines that do not parse
 * are skipped.
 */
void
LoadFlarmRangeEstimate(FlarmRangeEstimate &estimate, NLineReader &reader);
