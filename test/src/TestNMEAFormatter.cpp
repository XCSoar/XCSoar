// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Formatter/NMEAFormatter.hpp"
#include "util/StringAPI.hxx"
#include "TestUtil.hpp"
#include "NMEA/Info.hpp"
#include "Device/Parser.hpp"

int main()
{
  plan_tests(12);

  char buffer[100];
  NMEAParser parser;
  NMEAInfo info;

  info.Reset();
  info.clock = TimeStamp{FloatDuration{1}};

  // test formatter without valid GPS data
  FormatGPRMC(buffer, sizeof(buffer), info);
  ok1(StringIsEqual(buffer, 
                    "GPRMC,,V,,,,,,,,,"));

  FormatGPGSA(buffer, sizeof(buffer), info);
  ok1(StringIsEqual(buffer, 
                    "GPGSA,A,1,,,,,,,,,,,,,,,"));

  FormatGPGGA(buffer, sizeof(buffer), info);
  ok1(StringIsEqual(buffer, 
                    "GPGGA,,,,,,,,,,M,,,,"));

  FormatPGRMZ(buffer, sizeof(buffer), info);
  ok1(StringIsEqual(buffer,
                    "PGRMZ,,m,"));

  // parse NMEA sentences to set valid GPS data
  ok1(parser.ParseLine("$GPRMC,082311,A,5103.5403,N,00741.5742,E,055.3,022.4,230610,000.3,W*6C",
                       info));
  ok1(parser.ParseLine("$GPGSA,A,3,01,20,19,13,15,04,,,,,,,40.4,24.4,32.2*0A",
                       info));
  ok1(parser.ParseLine("$GPGGA,082311.0,5103.5403,N,00741.5742,E,2,06,24.4,18.893,M,,,,0000*09",
                       info));
  ok1(parser.ParseLine("$PGRMZ,2447,F,3*0E",
                       info));

  FormatGPRMC(buffer, sizeof(buffer), info);
  ok1(StringIsEqual(buffer, 
                    "GPRMC,082311.00,A,5103.540,N,00741.574,E,055.3,022.4,230610,000.3,W"));

  FormatGPGSA(buffer, sizeof(buffer), info);
  ok1(StringIsEqual(buffer, 
                    "GPGSA,A,3,01,20,19,13,15,04,,,,,,,40.4,24.4,32.2"));

  FormatGPGGA(buffer, sizeof(buffer), info);
  ok1(StringIsEqual(buffer,
                    "GPGGA,082311.00,5103.540,N,00741.574,E,2,06,24.4,18.893,M,,,,0000"));

  FormatPGRMZ(buffer, sizeof(buffer), info);
  ok1(StringIsEqual(buffer,
                    "PGRMZ,746,m,3"));

  return exit_status();
}
