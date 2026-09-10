// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/*
 * These tests describe what the NMEA checksum functions do today,
 * including the places where they are more permissive than NMEA 0183.
 * That is deliberate: #2127 replaces strtoul() and sprintf() here, and
 * without a record of the current behaviour there is no way to tell a
 * deliberate tightening from an accidental regression.  Where a test
 * pins something the standard does not allow, it says so.
 */

#include "NMEA/Checksum.hpp"
#include "TestUtil.hpp"
#include "util/StringAPI.hxx"

#include <string_view>

/* two sentences whose published checksums are widely quoted, so the
   expected values do not rest on this implementation */
static constexpr const char *GPGGA =
  "$GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,";
static constexpr const char *GPRMC =
  "$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E";

/* the checksum is computed at compile time */
static_assert(NMEAChecksum("$PX") == 0x08);
static_assert(NMEAChecksum(std::string_view{"$PX"}) == 0x08);

int main()
{
  plan_tests(49);

  /* --- NMEAChecksum(), const char * overload --------------------- */

  ok1(NMEAChecksum(GPGGA) == 0x47);
  ok1(NMEAChecksum(GPRMC) == 0x62);

  /* the leading '$' is not part of the checksum, and neither is the
     '!' that CAI302 sends instead */
  ok1(NMEAChecksum("$PX") == 0x08);
  ok1(NMEAChecksum("!PX") == 0x08);
  ok1(NMEAChecksum("PX") == 0x08);

  /* only the first character is skipped */
  ok1(NMEAChecksum("$P$X") == ('P' ^ '$' ^ 'X'));

  ok1(NMEAChecksum("") == 0x00);
  ok1(NMEAChecksum("$") == 0x00);

  /* a sentence can legitimately check out to zero */
  ok1(NMEAChecksum("$AA") == 0x00);

  /* every byte is folded in, including non-ASCII ones */
  ok1(NMEAChecksum("$\xff") == 0xff);

  /* --- NMEAChecksum(), std::string_view overload ----------------- */

  ok1(NMEAChecksum(std::string_view{GPGGA}) == 0x47);
  ok1(NMEAChecksum(std::string_view{"$PX"}) == 0x08);
  ok1(NMEAChecksum(std::string_view{"!PX"}) == 0x08);
  ok1(NMEAChecksum(std::string_view{}) == 0x00);
  ok1(NMEAChecksum(std::string_view{"$"}) == 0x00);

  /* the difference between the two overloads: a string_view carries
     its length, so an embedded NUL is data rather than the end */
  ok1(NMEAChecksum(std::string_view{"A\0B", 3}) == ('A' ^ 'B'));
  ok1(NMEAChecksum("A\0B") == 'A');

  /* the two agree on the part of a sentence before the asterisk,
     which is how VerifyNMEAChecksum() uses them */
  ok1(NMEAChecksum(std::string_view{GPRMC}) == NMEAChecksum(GPRMC));

  /* --- VerifyNMEAChecksum(), accepted ---------------------------- */

  ok1(VerifyNMEAChecksum("$GPGGA,123519,4807.038,N,01131.000,E,1,08,"
                         "0.9,545.4,M,46.9,M,,*47"));
  ok1(VerifyNMEAChecksum("$GPRMC,081836,A,3751.65,S,14507.36,E,000.0,"
                         "360.0,130998,011.3,E*62"));
  ok1(VerifyNMEAChecksum("$PX*08"));
  ok1(VerifyNMEAChecksum("!PX*08"));

  /* lower case hex digits are accepted */
  ok1(VerifyNMEAChecksum("$PGRMZ,2447,F,2*0f"));
  ok1(VerifyNMEAChecksum("$PGRMZ,2447,F,2*0F"));

  /* a sentence whose checksum is zero */
  ok1(VerifyNMEAChecksum("$AA*00"));

  /* the last asterisk separates, so one inside the payload is data */
  ok1(VerifyNMEAChecksum("$A*B*29"));

  /* --- VerifyNMEAChecksum(), rejected ---------------------------- */

  ok1(!VerifyNMEAChecksum("$PX*09"));      // wrong value
  ok1(!VerifyNMEAChecksum("$PX"));         // no asterisk
  ok1(!VerifyNMEAChecksum("$PX*"));        // nothing after it
  ok1(!VerifyNMEAChecksum(""));
  ok1(!VerifyNMEAChecksum("*08"));         // checksum of the empty string is 0
  ok1(!VerifyNMEAChecksum("$PX*ZZ"));      // not hex
  ok1(!VerifyNMEAChecksum("$PX*08X"));     // trailing garbage
  ok1(!VerifyNMEAChecksum("$PX*08 "));     // trailing space

  /* a value that cannot be a byte */
  ok1(!VerifyNMEAChecksum("$PX*100"));
  ok1(!VerifyNMEAChecksum("$PX*FFFF"));

  /* the caller has to strip the line ending first; NMEAReader does */
  ok1(!VerifyNMEAChecksum("$PX*08\r\n"));

  /* --- VerifyNMEAChecksum(), wider than the standard ------------- */

  /* NMEA 0183 says the checksum field is exactly two hex digits.
     strtoul() is looser than that, and these sentences verify even
     though a conforming talker would never send them.  Pinned so that
     replacing strtoul() shows up as a decision rather than a
     surprise; none of these has to keep working. */

  ok1(VerifyNMEAChecksum("$PX* 08"));      // leading space
  ok1(VerifyNMEAChecksum("$PX*\t08"));     // leading tab
  ok1(VerifyNMEAChecksum("$PX*+08"));      // explicit sign
  ok1(VerifyNMEAChecksum("$PX*0x08"));     // C hex prefix
  ok1(VerifyNMEAChecksum("$PX*008"));      // extra leading zero
  ok1(VerifyNMEAChecksum("$PX*8"));        // single digit
  ok1(VerifyNMEAChecksum("$AA*-0"));       // negative zero

  /* --- AppendNMEAChecksum() -------------------------------------- */

  char buffer[128];

  UnsafeCopyString(buffer, "$PX");
  AppendNMEAChecksum(buffer);
  ok1(StringIsEqual(buffer, "$PX*08"));

  /* two upper case digits, even when the value is small */
  UnsafeCopyString(buffer, "$AA");
  AppendNMEAChecksum(buffer);
  ok1(StringIsEqual(buffer, "$AA*00"));

  UnsafeCopyString(buffer, "$PGRMZ,2447,F,2");
  AppendNMEAChecksum(buffer);
  ok1(StringIsEqual(buffer, "$PGRMZ,2447,F,2*0F"));

  UnsafeCopyString(buffer, "");
  AppendNMEAChecksum(buffer);
  ok1(StringIsEqual(buffer, "*00"));

  /* what it appends is what the verifier accepts */
  bool round_trip = true;
  for (const char *sentence : {GPGGA, GPRMC, "$PX", "$AA",
                               "$LXWP0,N,,1000.0,0.0,,,,,,,0.0"}) {
    UnsafeCopyString(buffer, sentence);
    AppendNMEAChecksum(buffer);
    if (!VerifyNMEAChecksum(buffer))
      round_trip = false;
  }
  ok1(round_trip);

  return exit_status();
}
