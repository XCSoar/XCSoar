/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
             const GeoPoint &loc, const fixed speed,
             const Angle bearing, const fixed alt,
             const fixed baroalt, const fixed t)
{
  unsigned time = (unsigned)t;
  unsigned hour = time / 3600;
  time -= hour * 3600;
  unsigned minute = time / 60;
  time -= minute * 60;
  unsigned second = time;

  int lat_d, lat_m, lat_s;
  bool lat_sign;
  loc.latitude.ToDMS(lat_d, lat_m, lat_s, lat_sign);
  double lat_ms = lat_m + lat_s / 60.;

  int lon_d, lon_m, lon_s;
  bool lon_sign;
  loc.longitude.ToDMS(lon_d, lon_m, lon_s, lon_sign);
  double lon_ms = lon_m + lon_s / 60.;

  NarrowString<256> gprmc("$GPRMC");
  gprmc.AppendFormat(",%02d%02d%02d", hour, minute, second);
  gprmc.append(",A");
  gprmc.AppendFormat(",%02d%06.3f", lat_d, lat_ms);
  gprmc.append(lat_sign ? ",N" : ",S");
  gprmc.AppendFormat(",%03d%06.3f", lon_d, lon_ms);
  gprmc.append(lon_sign ? ",E" : ",W");
  gprmc.AppendFormat(",%.0f", (double)Units::ToUserUnit(speed, Unit::KNOTS));
  gprmc.AppendFormat(",%.0f", (double)bearing.Degrees());
  AppendNMEAChecksum(gprmc.buffer());

  writer.WriteLine(gprmc);
  printf("%s\n", gprmc.c_str());

  NarrowString<256> gpgga("$GPGGA");
  gpgga.AppendFormat(",%02d%02d%02d", hour, minute, second);
  gpgga.AppendFormat(",%02d%06.3f", lat_d, lat_ms);
  gpgga.append(lat_sign ? ",N" : ",S");
  gpgga.AppendFormat(",%03d%06.3f", lon_d, lon_ms);
  gpgga.append(lon_sign ? ",E" : ",W");
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

  const char *output_file = args.ExpectNext();
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
