/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "IO/TextWriter.hpp"
#include "Replay/IgcReplay.hpp"
#include "OS/PathName.hpp"
#include "Util/StaticString.hpp"
#include "NMEA/Checksum.hpp"
#include "Units/System.hpp"
#include "Args.hpp"

#include <stdio.h>

class IGCConverterReplay: public IgcReplay
{
  TextWriter writer;

public:
  IGCConverterReplay(const char *input_file, const char *output_file);

  bool HasError() {
    return writer.error();
  }

  virtual void on_reset() {}
  virtual void on_stop() {}
  virtual void on_bad_file() {}
  virtual void on_advance(const GeoPoint &loc,
                          const fixed speed, const Angle bearing,
                          const fixed alt, const fixed baroalt, const fixed t);
};

IGCConverterReplay::IGCConverterReplay(const char *input_file,
                                       const char *output_file)
  :writer(output_file)
{
  PathName path(input_file);
  SetFilename(path);
}

void
IGCConverterReplay::on_advance(const GeoPoint &loc, const fixed speed,
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
  gprmc.AppendFormat(",%.0f", (double)Units::ToUserUnit(speed, unKnots));
  gprmc.AppendFormat(",%.0f", (double)bearing.Degrees());
  AppendNMEAChecksum(gprmc.buffer());

  writer.writeln(gprmc);
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

  writer.writeln(gpgga);
  printf("%s\n", gpgga.c_str());

  NarrowString<256> pgrmz("$PGRMZ");
  pgrmz.AppendFormat(",%.0f,m", (double)baroalt);
  AppendNMEAChecksum(pgrmz.buffer());

  writer.writeln(pgrmz);
  printf("%s\n", pgrmz.c_str());
}

int
main(int argc, char **argv)
{
  Args args(argc, argv, "INFILE.igc OUTFILE.nmea");

  const char *input_file = args.ExpectNext();
  const char *output_file = args.ExpectNext();
  args.ExpectEnd();

  IGCConverterReplay replay(input_file, output_file);
  if (replay.HasError())
    return EXIT_FAILURE;

  replay.Start();
  while (replay.Update());

  return EXIT_SUCCESS;
}
