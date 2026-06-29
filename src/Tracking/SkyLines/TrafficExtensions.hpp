// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/ByteOrder.hxx"

#include "FLARM/Id.hpp"

#include <cstdint>

namespace SkyLinesTracking {

static constexpr uint32_t FLARM_EXTENSION_VALID = 0x80000000u;

/** OGN / cloud-server synthetic #TrafficResponsePacket::Traffic::pilot_id. */
static constexpr uint32_t OGN_PILOT_ID_MASK = 0x80000000u;

/**
 * Extension bits in #TrafficResponsePacket::Traffic::reserved /
 * reserved2.  Pre-extension clients ignore these fields and read only
 * pilot_id, time, location and altitude; all-zero reserved values must
 * therefore keep legacy semantics (altitude treated as valid).
 */
struct TrafficExtensions {
  unsigned track_deg = 0;
  bool track_valid = false;
  bool altitude_valid = true;
  unsigned aircraft_type = 0;
  FlarmId flarm_id = FlarmId::Undefined();

  struct WireFields {
    uint16_t reserved = 0;
    uint32_t reserved2 = 0;
  };

  [[gnu::const]]
  WireFields ToWire() const noexcept
  {
    WireFields w{};

    if (aircraft_type != 0)
      w.reserved = uint16_t((aircraft_type & 0x1Fu) << 9);

    if (track_valid && track_deg <= 359)
      w.reserved |= uint16_t(0x8000u | (track_deg & 0x1FFu));

    if (flarm_id.IsDefined())
      w.reserved2 = FLARM_EXTENSION_VALID |
        (flarm_id.Value() & 0xffffffu);

    /* Bit 14 marks altitude valid when other extension bits are set.
       When no extensions are present, leave reserved at zero so
       pre-extension clients and servers see the legacy wire form;
       #FromWire() still defaults altitude_valid to true. */
    const bool has_extension = w.reserved != 0 || w.reserved2 != 0;
    if (altitude_valid && has_extension)
      w.reserved |= 0x4000u;

    return w;
  }

  [[gnu::const]]
  static TrafficExtensions FromOgn(unsigned track_deg, bool track_valid,
                                   unsigned aircraft_type,
                                   uint32_t flarm_id, bool flarm_valid,
                                   bool altitude_valid) noexcept
  {
    TrafficExtensions e{};
    e.track_deg = track_deg;
    e.track_valid = track_valid;
    e.aircraft_type = aircraft_type;
    e.altitude_valid = altitude_valid;

    if (flarm_valid && flarm_id <= 0xFFFFFFu)
      e.flarm_id = FlarmId::FromValue(flarm_id);

    return e;
  }

  [[gnu::const]]
  static TrafficExtensions FromWire(uint16_t reserved_be,
                                    uint32_t reserved2_be) noexcept
  {
    const uint16_t reserved = FromBE16(reserved_be);
    const uint32_t reserved2 = FromBE32(reserved2_be);
    TrafficExtensions e{};

    const unsigned track = unsigned(reserved & 0x1ffu);
    if ((reserved & 0x8000u) != 0 && track < 360) {
      e.track_valid = true;
      e.track_deg = track;
    }

    e.aircraft_type = unsigned((reserved >> 9u) & 0x1fu);

    const bool ogn_extended = (reserved & 0x8000u) != 0 ||
      e.aircraft_type != 0 ||
      (reserved2 & FLARM_EXTENSION_VALID) != 0;
    if ((reserved & 0x4000u) != 0)
      e.altitude_valid = true;
    else if (ogn_extended)
      e.altitude_valid = false;

    if ((reserved2 & FLARM_EXTENSION_VALID) != 0)
      e.flarm_id = FlarmId::FromValue(reserved2 & 0xffffffu);

    return e;
  }
};

} // namespace SkyLinesTracking
