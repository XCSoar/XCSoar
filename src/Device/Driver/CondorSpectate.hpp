// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "system/Path.hpp"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"

struct MoreData;
struct DerivedInfo;

struct CondorSpectateReference {
  double latitude = 0;
  double longitude = 0;
  double altitude = 0;
  bool defined = false;
};

/**
 * Build FLARM NMEA sentences from a Condor 3 Spectate.json snapshot.
 */
class CondorSpectateBuilder {
  static constexpr std::size_t max_lines = 256;
  static constexpr std::size_t max_line_length = 128;

public:
  using LineBuffer = StaticString<max_line_length>;
  using Lines = StaticArray<LineBuffer, max_lines>;

  /**
   * Read @p path and append NMEA lines to @p lines.
   *
   * @param own_cn if non-empty, exclude this competition number from
   * traffic list
   * @param live_ref when defined, use Condor3UDP position as the
   * coordinate reference instead of own-ship from Spectate.json
   */
  static bool Build(Path path, const char *own_cn, Lines &lines,
                    const CondorSpectateReference *live_ref=nullptr) noexcept;
};

class CondorSpectateDevice final : public AbstractDevice {
  CondorSpectateReference live_ref;

public:
  void OnCalculatedUpdate(const MoreData &basic,
                          const DerivedInfo &calculated) override;

  [[gnu::pure]]
  const CondorSpectateReference &GetLiveReference() const noexcept {
    return live_ref;
  }
};

extern const struct DeviceRegister condor_spectate_driver;
