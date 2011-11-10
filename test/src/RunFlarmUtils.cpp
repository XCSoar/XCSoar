/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "OS/PathName.hpp"
#include "ConsoleOperationEnvironment.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Util/StringUtil.hpp"

#ifdef HAVE_POSIX
#include "Device/Port/TTYPort.hpp"
#else
#include "Device/Port/SerialPort.hpp"
#endif

#include <stdio.h>

static void
ChangePilot(FlarmDevice &flarm)
{
  fprintf(stdout, "Please enter the new pilot name:\n");
  while (true) {
    fprintf(stdout, "> ");

    char pilot_name[64];
    if (fgets(pilot_name, 64, stdin) == NULL || strlen(pilot_name) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    TrimRight(pilot_name);
    fprintf(stdout, "Setting pilot name to \"%s\" ...\n", pilot_name);

    if (flarm.SetPilot(PathName(pilot_name)))
      fprintf(stdout, "Pilot name set to \"%s\"\n", pilot_name);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangePlaneType(FlarmDevice &flarm)
{
  fprintf(stdout, "Please enter the new plane type:\n");
  while (true) {
    fprintf(stdout, "> ");

    char plane_type[64];
    if (fgets(plane_type, 64, stdin) == NULL || strlen(plane_type) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    TrimRight(plane_type);
    fprintf(stdout, "Setting plane type to \"%s\" ...\n", plane_type);

    if (flarm.SetPlaneType(PathName(plane_type)))
      fprintf(stdout, "Plane type set to \"%s\"\n", plane_type);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeRegistration(FlarmDevice &flarm)
{
  fprintf(stdout, "Please enter the new plane registration:\n");
  while (true) {
    fprintf(stdout, "> ");

    char registration[64];
    if (fgets(registration, 64, stdin) == NULL || strlen(registration) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    TrimRight(registration);
    fprintf(stdout, "Setting plane registration to \"%s\" ...\n", registration);

    if (flarm.SetPlaneRegistration(PathName(registration)))
      fprintf(stdout, "Plane registration set to \"%s\"\n", registration);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
WriteMenu()
{
  fprintf(stdout, "--------------------------------\n"
                  "Flarm Utils Menu\n"
                  "--------------------------------\n"
                  "Press any of the following keys:\n\n"
                  "h: Display this menu\n"
                  "1: Change pilot name\n"
                  "2: Change plane type\n"
                  "3: Change plane registration\n"
                  "q: Quit this application\n"
                  "--------------------------------\n");
}

static void
RunUI(FlarmDevice &flarm)
{
  WriteMenu();

  while (true) {
    fprintf(stdout, "> ");

    char in[20];
    if (fgets(in, 20, stdin) == NULL || strlen(in) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    switch (in[0]) {
    case 'h':
    case 'H':
      WriteMenu();
      break;
    case '1':
      ChangePilot(flarm);
      break;
    case '2':
      ChangePlaneType(flarm);
      break;
    case '3':
      ChangeRegistration(flarm);
      break;
    case 'q':
    case 'Q':
      fprintf(stdout, "Closing Flarm Utils ...\n");
      return;
    default:
      fprintf(stdout, "Invalid input\n");
      break;
    }
  }
}
static Waypoint
MakeWaypoint(const TCHAR *name, int altitude,
             double longitude, double latitude)
{
  const GeoPoint gp(Angle::Degrees(fixed(7.7061111111111114)),
                    Angle::Degrees(fixed(51.051944444444445)));
  Waypoint wp(GeoPoint(Angle::Degrees(fixed(longitude)),
                       Angle::Degrees(fixed(latitude))));
  wp.name = name;
  wp.altitude = fixed(altitude);
  return wp;
}

int
main(int argc, char **argv)
{
  if (argc != 3) {
    fprintf(stderr, "Usage: %s PORT BAUD\n", argv[0]);
    return EXIT_FAILURE;
  }

  PathName port_name(argv[1]);

  DeviceConfig config;
  config.Clear();
  config.baud_rate = atoi(argv[2]);

#ifdef HAVE_POSIX
  TTYPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#else
  SerialPort port(port_name, config.baud_rate, *(Port::Handler *)NULL);
#endif

  if (!port.Open()) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  Declaration declaration(NULL);
  declaration.pilot_name = _T("Foo Bar");
  declaration.aircraft_type = _T("Cirrus");
  declaration.aircraft_registration = _T("D-3003");
  declaration.competition_id = _T("33");

  declaration.Append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));
  declaration.Append(MakeWaypoint(_T("Foo"), 488, 8, 52));
  declaration.Append(MakeWaypoint(_T("Bar"), 488, 7.5, 50));
  declaration.Append(MakeWaypoint(_T("Bergneustadt"), 488,
                                  7.7061111111111114, 51.051944444444445));

  port.SetRxTimeout(500);
  FlarmDevice flarm(port);
  RunUI(flarm);

  return EXIT_SUCCESS;
}
