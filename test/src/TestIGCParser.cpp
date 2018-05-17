/* Copyright_License {

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

#include "IGC/IGCParser.hpp"
#include "IGC/IGCExtensions.hpp"
#include "IGC/IGCFix.hpp"
#include "IGC/IGCHeader.hpp"
#include "IGC/IGCDeclaration.hpp"
#include "Time/BrokenDate.hpp"
#include "Time/BrokenTime.hpp"
#include "TestUtil.hpp"

#include <string.h>

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
  ok1(strcmp(header.id, "A3Z") == 0);
  ok1(header.flight == 1);
}

static void
TestExtensions()
{
  IGCExtensions extensions;
  ok1(!IGCParseExtensions("", extensions));
  ok1(!IGCParseExtensions("B1122385103117N00742367EA004900048700000", extensions));
  ok1(!IGCParseExtensions("AXYZAA", extensions));

  ok1(IGCParseExtensions("I043638FXA3941ENL4246GSP4749TRT", extensions));
  ok1(extensions.size() == 4);
  ok1(extensions[0].start == 36);
  ok1(extensions[0].finish == 38);
  ok1(strcmp(extensions[0].code, "FXA") == 0);
  ok1(extensions[1].start == 39);
  ok1(extensions[1].finish == 41);
  ok1(strcmp(extensions[1].code, "ENL") == 0);
  ok1(extensions[2].start == 42);
  ok1(extensions[2].finish == 46);
  ok1(strcmp(extensions[2].code, "GSP") == 0);
  ok1(extensions[3].start == 47);
  ok1(extensions[3].finish == 49);
  ok1(strcmp(extensions[3].code, "TRT") == 0);
}

static void
TestDate()
{
  BrokenDate date;
  ok1(!IGCParseDateRecord("", date));
  ok1(!IGCParseDateRecord("B1122385103117N00742367EA004900048700000", date));
  ok1(!IGCParseDateRecord("HFDTEXXX", date));

  ok1(IGCParseDateRecord("HFDTE040910", date));
  ok1(date.year == 2010);
  ok1(date.month == 9);
  ok1(date.day == 4);

  ok1(IGCParseDateRecord("HFDTE010100", date));
  ok1(date.year == 2000);
  ok1(date.month == 1);
  ok1(date.day == 1);

  ok1(IGCParseDateRecord("HFDTE311299", date));
  ok1(date.year == 1999);
  ok1(date.month == 12);
  ok1(date.day == 31);
}

static void
TestLocation()
{
  GeoPoint location;
  ok1(!IGCParseLocation("", location));
  ok1(!IGCParseLocation("9103117N00742367E", location));
  ok1(!IGCParseLocation("-203117N00742367E", location));
  ok1(!IGCParseLocation("5103117N20742367E", location));

  ok1(IGCParseLocation("5103117N00742367E", location));
  ok1(equals(location, 51.05195, 7.706116667));

  ok1(IGCParseLocation("1234500S12345678E", location));
  ok1(equals(location, -12.575, 123.7613));
  ok1(IGCParseLocation("1234500N12345678W", location));
  ok1(equals(location, 12.575, -123.7613));
  ok1(IGCParseLocation("1234500S12345678W", location));
  ok1(equals(location, -12.575, -123.7613));
}

static void
TestFix()
{
  IGCExtensions extensions;
  extensions.clear();

  IGCFix fix;
  ok1(!IGCParseFix("", extensions, fix));
  ok1(!IGCParseFix("B1122385103117N00742367EA", extensions, fix));

  ok1(!IGCParseFix("B1122385103117X00742367EA0049000487", extensions, fix));
  ok1(!IGCParseFix("B1122385103117N00742367XA0049000487", extensions, fix));
  ok1(!IGCParseFix("B1122389003117N00742367EA0049000487", extensions, fix));
  ok1(!IGCParseFix("B1122385103117N18042367EA0049000487", extensions, fix));
  ok1(!IGCParseFix("B1122385163117N00742367EA0049000487", extensions, fix));
  ok1(!IGCParseFix("B1122385103117N00762367EA0049000487", extensions, fix));

  ok1(IGCParseFix("B1122385103117N00742367EA0049000487", extensions, fix));
  ok1(fix.time == BrokenTime(11, 22, 38));
  ok1(equals(fix.location, 51.05195, 7.70611667));
  ok1(fix.gps_valid);
  ok1(fix.pressure_altitude == 490);
  ok1(fix.gps_altitude == 487);

  ok1(IGCParseFix("B1122385103117N00742367EV0049000487", extensions, fix));
  ok1(fix.time == BrokenTime(11, 22, 38));
  ok1(equals(fix.location, 51.05195, 7.70611667));
  ok1(!fix.gps_valid);
  ok1(fix.pressure_altitude == 490);
  ok1(fix.gps_altitude == 487);

  ok1(!IGCParseFix("B1122385103117N00742367EX0049000487", extensions, fix));

  ok1(IGCParseFix("B1122435103117N00742367EA004900000000000",
                  extensions, fix));
  ok1(fix.time == BrokenTime(11, 22, 43));
  ok1(fix.gps_valid);
  ok1(fix.pressure_altitude == 490);
  ok1(fix.gps_altitude == 0);

  ok1(IGCParseFix("B1122535103117S00742367WA104900000700000",
                  extensions, fix));
  ok1(fix.time == BrokenTime(11, 22, 53));
  ok1(fix.gps_valid);
  ok1(equals(fix.location, -51.05195, -7.70611667));
  ok1(fix.pressure_altitude == 10490);
  ok1(fix.gps_altitude == 7);
}

static void
TestFixTime()
{
  BrokenTime time;
  ok1(!IGCParseTime("", time));

  ok1(IGCParseTime("000000", time));
  ok1(time == BrokenTime(00, 00, 00));

  ok1(IGCParseTime("112238", time));
  ok1(time == BrokenTime(11, 22, 38));

  ok1(IGCParseTime("235959", time));
  ok1(time == BrokenTime(23, 59, 59));

  ok1(!IGCParseTime("235960", time));
  ok1(!IGCParseTime("236059", time));
  ok1(!IGCParseTime("240000", time));

  ok1(IGCParseTime("0123375103117N00742367EV0049000487", time));
  ok1(time == BrokenTime(01, 23, 37));
}

static void
TestDeclarationHeader()
{
  IGCDeclarationHeader header;
  ok1(!IGCParseDeclarationHeader("", header));
  ok1(!IGCParseDeclarationHeader("C0309111642280309110001-2", header));

  ok1(IGCParseDeclarationHeader("C020811084345000000000002Task", header));
  ok1(header.datetime.day == 2);
  ok1(header.datetime.month == 8);
  ok1(header.datetime.year == 2011);
  ok1(header.datetime.hour == 8);
  ok1(header.datetime.minute == 43);
  ok1(header.datetime.second == 45);
  ok1(!header.flight_date.IsPlausible());
  ok1(header.num_turnpoints == 2);
  ok1(header.task_id[0] == '0' &&
      header.task_id[1] == '0' &&
      header.task_id[2] == '0' &&
      header.task_id[3] == '0');
  ok1(header.task_name == "Task");

  ok1(IGCParseDeclarationHeader("C210706120510210706000103", header));
  ok1(header.datetime.day == 21);
  ok1(header.datetime.month == 7);
  ok1(header.datetime.year == 2006);
  ok1(header.datetime.hour == 12);
  ok1(header.datetime.minute == 5);
  ok1(header.datetime.second == 10);
  ok1(header.flight_date.day == 21);
  ok1(header.flight_date.month == 7);
  ok1(header.flight_date.year == 2006);
  ok1(header.num_turnpoints == 3);
  ok1(header.task_id[0] == '0' &&
      header.task_id[1] == '0' &&
      header.task_id[2] == '0' &&
      header.task_id[3] == '1');
  ok1(header.task_name.empty());
}

static void
TestDeclarationTurnpoint()
{
  IGCDeclarationTurnpoint tp;
  ok1(!IGCParseDeclarationTurnpoint("", tp));

  ok1(IGCParseDeclarationTurnpoint("C5000633N01015083ESchweinfurtsued", tp));
  ok1(equals(tp.location, 50.01055, 10.251383333));
  ok1(tp.name == "Schweinfurtsued");

  ok1(IGCParseDeclarationTurnpoint("C5100633S00015083W", tp));
  ok1(equals(tp.location, -51.01055, -0.251383333));
  ok1(tp.name.empty());
}

int main(int argc, char **argv)
{
  plan_tests(136);

  TestHeader();
  TestDate();
  TestLocation();
  TestExtensions();
  TestFix();
  TestFixTime();
  TestDeclarationHeader();
  TestDeclarationTurnpoint();

  return exit_status();
}
