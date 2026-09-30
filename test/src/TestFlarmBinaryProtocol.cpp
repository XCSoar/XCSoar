// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/FLARM/BinaryProtocol.hpp"
#include "TestUtil.hpp"

static bool
FeedAll(FLARM::PFLAXNotSupportedMatcher &matcher,
        const char *text) noexcept
{
  bool found = false;
  for (const char *p = text; *p != '\0'; ++p)
    found = matcher.Feed(std::byte(static_cast<unsigned char>(*p))) || found;
  return found;
}

static void
TestPFLAXNotSupported()
{
  FLARM::PFLAXNotSupportedMatcher matcher;

  /* PowerFLARM Flex: NMEA keeps flowing, then the refusal. */
  ok1(FeedAll(matcher,
              "$GPRMC,133100,V,,,,,,,,,,N*00\r\n"
              "$PFLAX,A,ERROR,NOTSUPPORTED*6D\r\n"));

  /* BLE answers $PFLAX,A and does not switch.  That is not a refusal
     sentence, so the binary pings still run. */
  matcher.Reset();
  ok1(!FeedAll(matcher, "$PFLAX,A*2E\r\n"));

  /* A broken prefix must not stick across the real sentence. */
  matcher.Reset();
  ok1(!FeedAll(matcher, "$PFLAX,A,ERRX"));
  ok1(FeedAll(matcher, "$PFLAX,A,ERROR,NOTSUPPORTED"));

  /* Split across two reads, as the port returns one byte at a time. */
  matcher.Reset();
  ok1(!FeedAll(matcher, "$PFLAX,A,ERROR,NOT"));
  ok1(FeedAll(matcher, "SUPPORTED"));
}

int main()
{
  plan_tests(13 + 6);

  TestPFLAXNotSupported();

  ok1(FLARM::PROTOCOL_VERSION == 1);
  ok1(FLARM::MAX_IGC_DOWNLOAD_ATTEMPTS == 2);

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
