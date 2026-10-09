// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FLARM/Id.hpp"
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
 * Every position report of a FLARM target counts once, with its
 * distance and its bearing relative to the own ground track.
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
    uint32_t count = 0;

    /** the largest distance received [m] */
    uint32_t maximum = 0;

    std::array<uint32_t, BINS> histogram{};

    void Add(unsigned distance) noexcept;

    /**
     * The distance below which the fraction @p p of the reports lie,
     * rounded up to the histogram resolution [m].
     */
    [[gnu::pure]]
    std::optional<unsigned> Percentile(double p) const noexcept;
  };

  std::array<Sector, SECTORS> sectors;

  /** the first and last report counted */
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

  void Add(Angle relative_bearing, unsigned distance,
           std::optional<TimePoint> time) noexcept;
};

/**
 * Feeds the FLARM traffic of each update into a #FlarmRangeEstimate,
 * counting each position report once.
 */
class FlarmRangeEstimator {
  struct Seen {
    FlarmId id;
    Validity valid;
  };

  /** the last report counted per target in the traffic list */
  StaticArray<Seen, 32> seen;

public:
  /**
   * @param flying only reports received in flight are counted
   */
  void Process(FlarmRangeEstimate &estimate, const NMEAInfo &basic,
               bool flying) noexcept;
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
