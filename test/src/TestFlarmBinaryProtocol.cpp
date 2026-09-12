// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/FLARM/BinaryProtocol.hpp"
#include "TestUtil.hpp"

int main()
{
  plan_tests(12);

  ok1(FLARM::PROTOCOL_VERSION == 1);

  /* spec-correct little-endian seqNo (GETIGCDATA NACK 238, list NACK 68) */
  const std::byte ee_00[]{std::byte{0xee}, std::byte{0x00}};
  ok1(FLARM::AckSequenceMatches(238, ee_00, true));
  ok1(FLARM::AckSequenceMatches(238, ee_00, false));

  const std::byte rec_44[]{std::byte{0x44}, std::byte{0x00}};
  ok1(FLARM::AckSequenceMatches(68, rec_44, true));

  /* PowerMouse: seq_hi is not the request high byte */
  const std::byte be_80[]{std::byte{0xbe}, std::byte{0x80}};
  ok1(FLARM::AckSequenceMatches(190, be_80, true));
  ok1(!FLARM::AckSequenceMatches(190, be_80, false));

  const std::byte nack_15[]{std::byte{0x15}, std::byte{0x04}};
  ok1(FLARM::AckSequenceMatches(21, nack_15, true));

  const std::byte be_00[]{std::byte{0xbe}, std::byte{0x00}};
  ok1(FLARM::AckSequenceMatches(190, be_00, false));
  ok1(FLARM::AckSequenceMatches(190, be_00, true));

  /* extra NACK data after a real seqNo: no lo-byte fallback */
  const std::byte be_80_extra[]{
    std::byte{0xbe}, std::byte{0x80}, std::byte{0x00},
  };
  ok1(!FLARM::AckSequenceMatches(190, be_80_extra, true));

  const std::byte short_seq[]{std::byte{0xbe}};
  ok1(!FLARM::AckSequenceMatches(190, short_seq, true));
  ok1(!FLARM::AckSequenceMatches(190, {}, true));

  return exit_status();
}
