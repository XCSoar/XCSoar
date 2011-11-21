/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Replay/IGCParser.hpp"
#include "DateTime.hpp"
#include "TestUtil.hpp"

static void
TestHeader()
{
  IGCHeader header;
  ok1(!IGCParseHeader("", header));
  ok1(!IGCParseHeader("B1122385103117N00742367EA004900048700000", header));
  ok1(!IGCParseHeader("AXYZAA", header));

  ok1(IGCParseHeader("AXCSfoo", header));
  ok1(strcmp(header.manufacturer, "XCS") == 0);
  ok1(strcmp(header.id, "foo") == 0);
  ok1(header.flight == 0);

  ok1(IGCParseHeader("ACAM3OV", header));
  ok1(strcmp(header.manufacturer, "CAM") == 0);
  ok1(strcmp(header.id, "3OV") == 0);
  ok1(header.flight == 0);

  ok1(IGCParseHeader("ALXN13103FLIGHT:1", header));
  ok1(strcmp(header.manufacturer, "LXN") == 0);
  ok1(strcmp(header.id, "a3z") == 0);
  ok1(header.flight == 1);
}

static void
TestDate()
{
  BrokenDate date;
  ok1(!IGCParseDate("", date));
  ok1(!IGCParseDate("B1122385103117N00742367EA004900048700000", date));
  ok1(!IGCParseDate("HFDTEXXX", date));

  ok1(IGCParseDate("HFDTE040910", date));
  ok1(date.year == 2010);
  ok1(date.month == 9);
  ok1(date.day == 4);

  ok1(IGCParseDate("HFDTE010100", date));
  ok1(date.year == 2000);
  ok1(date.month == 1);
  ok1(date.day == 1);

  ok1(IGCParseDate("HFDTE311299", date));
  ok1(date.year == 2099);
  ok1(date.month == 12);
  ok1(date.day == 31);
}

static void
TestFix()
{
  IGCFix fix;
  ok1(!IGCParseFix("", fix));
  ok1(!IGCParseFix("B1122385103117N00742367EA", fix));

  ok1(IGCParseFix("B1122385103117N00742367EA0049000487", fix));
  ok1(fix.time == BrokenTime(11, 22, 38));
  ok1(equals(fix.location, 51.05195, 7.70611667));
  ok1(equals(fix.pressure_altitude, 490));
  ok1(equals(fix.gps_altitude, 487));

  ok1(IGCParseFix("B1122435103117N00742367EA004900000000000", fix));
  ok1(fix.time == BrokenTime(11, 22, 43));
  ok1(equals(fix.pressure_altitude, 490));
  ok1(equals(fix.gps_altitude, 490));

  ok1(IGCParseFix("B1122535103117S00742367WA104900000700000", fix));
  ok1(fix.time == BrokenTime(11, 22, 53));
  ok1(equals(fix.location, -51.05195, -7.70611667));
  ok1(equals(fix.pressure_altitude, 10490));
  ok1(equals(fix.gps_altitude, 7));
}

int main(int argc, char **argv)
{
  plan_tests(46);

  TestHeader();
  TestDate();
  TestFix();

  return exit_status();
}
