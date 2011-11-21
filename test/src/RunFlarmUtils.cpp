/*
Copyright_License {

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
  while (true) {
    fprintf(stdout, "Please enter the new pilot name:\n");
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
ChangeCoPilot(FlarmDevice &flarm)
{
  while (true) {
    fprintf(stdout, "Please enter the new copilot name:\n");
    fprintf(stdout, "> ");

    char copilot_name[64];
    if (fgets(copilot_name, 64, stdin) == NULL || strlen(copilot_name) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    TrimRight(copilot_name);
    fprintf(stdout, "Setting copilot name to \"%s\" ...\n", copilot_name);

    if (flarm.SetCoPilot(PathName(copilot_name)))
      fprintf(stdout, "CoPilot name set to \"%s\"\n", copilot_name);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangePlaneType(FlarmDevice &flarm)
{
  while (true) {
    fprintf(stdout, "Please enter the new plane type:\n");
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
  while (true) {
    fprintf(stdout, "Please enter the new plane registration:\n");
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
ChangeCompetitionId(FlarmDevice &flarm)
{
  while (true) {
    fprintf(stdout, "Please enter the new competition id:\n");
    fprintf(stdout, "> ");

    char id[64];
    if (fgets(id, 64, stdin) == NULL || strlen(id) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    TrimRight(id);
    fprintf(stdout, "Setting competition id to \"%s\" ...\n", id);

    if (flarm.SetCompetitionId(PathName(id)))
      fprintf(stdout, "competition id set to \"%s\"\n", id);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeCompetitionClass(FlarmDevice &flarm)
{
  while (true) {
    fprintf(stdout, "Please enter the new competition class:\n");
    fprintf(stdout, "> ");

    char comp_class[64];
    if (fgets(comp_class, 64, stdin) == NULL || strlen(comp_class) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    TrimRight(comp_class);
    fprintf(stdout, "Setting competition class to \"%s\" ...\n", comp_class);

    if (flarm.SetCompetitionClass(PathName(comp_class)))
      fprintf(stdout, "Competition class set to \"%s\"\n", comp_class);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeRange(FlarmDevice &flarm)
{
  while (true) {
    fprintf(stdout, "Please enter the range setting (2000-25500):\n");
    fprintf(stdout, "> ");

    char range[64];
    if (fgets(range, 64, stdin) == NULL || strlen(range) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    TrimRight(range);

    char *end_ptr;
    unsigned num_range = strtoul(range, &end_ptr, 10);
    if (range == end_ptr) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    fprintf(stdout, "Setting range to \"%d\" ...\n", num_range);

    if (flarm.SetRange(num_range))
      fprintf(stdout, "Range set to \"%d\"\n", num_range);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
WriteMenu()
{
  fprintf(stdout, "------------------------------------\n"
                  "Flarm Utils Menu\n"
                  "------------------------------------\n"
                  "Press any of the following commands:\n\n"
                  "h:  Display this menu\n"
                  "1:  Change pilot name\n"
                  "2:  Change copilot name\n"
                  "3:  Change plane type\n"
                  "4:  Change plane registration\n"
                  "5:  Change competition id\n"
                  "6:  Change competition class\n"
                  "7:  Change receiving range\n"
                  "r:  Restart the FLARM\n"
                  "s+: Enable the stealth mode\n"
                  "s-: Disable the stealth mode\n"
                  "q:  Quit this application\n"
                  "------------------------------------\n");
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
    case '?':
    case 'h':
    case 'H':
      WriteMenu();
      break;
    case '1':
      ChangePilot(flarm);
      break;
    case '2':
      ChangeCoPilot(flarm);
      break;
    case '3':
      ChangePlaneType(flarm);
      break;
    case '4':
      ChangeRegistration(flarm);
      break;
    case '5':
      ChangeCompetitionId(flarm);
      break;
    case '6':
      ChangeCompetitionClass(flarm);
      break;
    case '7':
      ChangeRange(flarm);
      break;
    case 'r':
    case 'R':
      fprintf(stdout, "Restarting the FLARM ...\n");
      flarm.Restart();
      break;
    case 's':
    case 'S':
      if (strlen(in) < 2 || (in[1] != '+' && in[1] != '-')) {
        fprintf(stdout, "Invalid input\n");
        break;
      }

      fprintf(stdout, "Changing stealth mode setting ...\n");
      if (flarm.SetStealthMode(in[1] == '+')) {
        if (in[1] == '+')
          fprintf(stdout, "Stealth mode enabled\n");
        else
          fprintf(stdout, "Stealth mode disabled\n");
      } else
        fprintf(stdout, "Operation failed!\n");
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

  port.SetRxTimeout(500);
  FlarmDevice flarm(port);
  RunUI(flarm);

  return EXIT_SUCCESS;
}
