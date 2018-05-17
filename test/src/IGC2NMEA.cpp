/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "DebugReplay.hpp"
#include "IO/TextWriter.hpp"
#include "NMEA/Checksum.hpp"
#include "Units/System.hpp"
#include "OS/Args.hpp"

#include <stdio.h>

static void
GenerateNMEA(TextWriter &writer,
             const GeoPoint &loc, const double speed,
             const Angle bearing, const double alt,
             const double baroalt, const double t)
{
  unsigned time = (unsigned)t;
  unsigned hour = time / 3600;
  time -= hour * 3600;
  unsigned minute = time / 60;
  time -= minute * 60;
  unsigned second = time;

  const auto lat = loc.latitude.ToDMS();
  double lat_ms = lat.minutes + lat.seconds / 60.;

  const auto lon = loc.longitude.ToDMS();
  double lon_ms = lon.minutes + lon.seconds / 60.;

  NarrowString<256> gprmc("$GPRMC");
  gprmc.AppendFormat(",%02d%02d%02d", hour, minute, second);
  gprmc.append(",A");
  gprmc.AppendFormat(",%02d%06.3f", lat.degrees, lat_ms);
  gprmc.append(lat.negative ? ",S" : ",N");
  gprmc.AppendFormat(",%03d%06.3f", lon.degrees, lon_ms);
  gprmc.append(lon.negative ? ",W" : ",E");
  gprmc.AppendFormat(",%.0f", (double)Units::ToUserUnit(speed, Unit::KNOTS));
  gprmc.AppendFormat(",%.0f", (double)bearing.Degrees());
  AppendNMEAChecksum(gprmc.buffer());

  writer.WriteLine(gprmc);
  printf("%s\n", gprmc.c_str());

  NarrowString<256> gpgga("$GPGGA");
  gpgga.AppendFormat(",%02d%02d%02d", hour, minute, second);
  gpgga.AppendFormat(",%02d%06.3f", lat.degrees, lat_ms);
  gprmc.append(lat.negative ? ",S" : ",N");
  gpgga.AppendFormat(",%03d%06.3f", lon.degrees, lon_ms);
  gprmc.append(lon.negative ? ",W" : ",E");
  gpgga.append(",1,,");
  gpgga.AppendFormat(",%.0f,m", (double)alt);
  AppendNMEAChecksum(gpgga.buffer());

  writer.WriteLine(gpgga);
  printf("%s\n", gpgga.c_str());

  NarrowString<256> pgrmz("$PGRMZ");
  pgrmz.AppendFormat(",%.0f,m", (double)baroalt);
  AppendNMEAChecksum(pgrmz.buffer());

  writer.WriteLine(pgrmz);
  printf("%s\n", pgrmz.c_str());
}

int
main(int argc, char **argv)
{
  Args args(argc, argv, "INFILE.igc OUTFILE.nmea");
  DebugReplay *replay = CreateDebugReplay(args);
  if (replay == NULL)
    return EXIT_FAILURE;

  const auto output_file = args.ExpectNextPath();
  args.ExpectEnd();

  TextWriter writer(output_file);
  if (!writer.IsOpen()) {
    fprintf(stderr, "Failed to create output file\n");
    return EXIT_FAILURE;
  }

  while (replay->Next()) {
    const NMEAInfo &basic = replay->Basic();
    GenerateNMEA(writer, basic.location,
                 basic.ground_speed, basic.track, basic.gps_altitude,
                 basic.baro_altitude, basic.time);
  }

  return EXIT_SUCCESS;
}
