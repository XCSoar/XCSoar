// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DebugReplay.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "NMEA/Checksum.hpp"
#include "Units/System.hpp"
#include "system/Args.hpp"
#include "util/PrintException.hxx"
#include "Formatter/NMEAFormatter.hpp"

#include <stdio.h>

static bool
GenerateNMEA(BufferedOutputStream &os,
             const NMEAInfo basic) noexcept
{
  char gprmc_buffer[100];
  FormatGPRMC(gprmc_buffer, sizeof(gprmc_buffer), basic);
  StaticString<256> gprmc("$");
  gprmc.append(gprmc_buffer);
  if (!AppendNMEAChecksum(gprmc.buffer(), gprmc.capacity()))
    return false;

  os.Write(gprmc);
  os.NewLine();

  char gpgga_buffer[100];
  FormatGPGGA(gpgga_buffer, sizeof(gpgga_buffer), basic);
  StaticString<256> gpgga("$");
  gpgga.append(gpgga_buffer);
  if (!AppendNMEAChecksum(gpgga.buffer(), gpgga.capacity()))
    return false;

  os.Write(gpgga);
  os.NewLine();

  char pgrmz_buffer[100];
  FormatPGRMZ(pgrmz_buffer, sizeof(pgrmz_buffer), basic);
  StaticString<256> pgrmz("$");
  pgrmz.append(pgrmz_buffer);
  if (!AppendNMEAChecksum(pgrmz.buffer(), pgrmz.capacity()))
    return false;

  os.Write(pgrmz);
  os.NewLine();

  return true;
}

int
main(int argc, char **argv) noexcept
try {
  Args args(argc, argv, "INFILE.igc OUTFILE.nmea");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  const auto output_file = args.ExpectNextPath();
  args.ExpectEnd();

  FileOutputStream fos{output_file};
  BufferedOutputStream bos{fos};

  while (replay->Next()) {
    const NMEAInfo &basic = replay->Basic();
    if (!GenerateNMEA(bos, basic))
      return EXIT_FAILURE;
  }

  bos.Flush();
  fos.Commit();

  return EXIT_SUCCESS;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
