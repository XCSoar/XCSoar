// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "system/Path.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"

struct MoreData;
struct DerivedInfo;

struct Condor3SpectateReference {
  double latitude = 0;
  double longitude = 0;
  double altitude = 0;
  bool defined = false;
};

/**
 * Build FLARM NMEA sentences from a Condor 3 Spectate.json snapshot.
 */
class Condor3SpectateBuilder {
  /* Up to max_players × (4×PFLAM + PFLAA) + PFLAU. */
  static constexpr std::size_t max_lines = 384;
  static constexpr std::size_t max_line_length = 128;

public:
  using LineBuffer = StaticString<max_line_length>;
  using Lines = StaticArray<LineBuffer, max_lines>;

  /**
   * Read @p path and append NMEA lines to @p lines.
   *
   * @param own_cn if non-empty, exclude this competition number from
   * traffic list
   * @param live_ref when defined, use the live GPS position for
   * PFLAA north/east.  Relative altitude still uses Spectate.json
   * own-ship when the competition number is found, so GPS geoid
   * and Condor altimeter heights are not mixed.
   */
  static bool Build(Path path, const char *own_cn, Lines &lines,
                    const Condor3SpectateReference *live_ref=nullptr) noexcept;
};

class Condor3SpectateDevice final : public AbstractDevice {
  Condor3SpectateReference live_ref;

public:
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;

  [[gnu::pure]]
  const Condor3SpectateReference &GetLiveReference() const noexcept {
    return live_ref;
  }
};

extern const struct DeviceRegister condor3_spectate_driver;
