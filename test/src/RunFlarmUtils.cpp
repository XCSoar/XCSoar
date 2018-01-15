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

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "Device/Config.hpp"
#include "OS/Args.hpp"
#include "Util/StringUtil.hpp"
#include "Util/ConvertString.hpp"
#include "Util/PrintException.hxx"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "IO/Async/GlobalAsioThread.hpp"
#include "IO/Async/AsioThread.hpp"

#include <stdio.h>

static void
ChangePilot(FlarmDevice &flarm, OperationEnvironment &env)
{
  while (true) {
    TCHAR old_pilot_name[64];
    if (flarm.GetPilot(old_pilot_name, 64, env))
      _tprintf(_T("Old pilot name: \"%s\"\n"), old_pilot_name);

    fprintf(stdout, "Please enter the new pilot name:\n");
    fprintf(stdout, "> ");

    char pilot_name[64];
    if (fgets(pilot_name, 64, stdin) == NULL || strlen(pilot_name) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    StripRight(pilot_name);
    fprintf(stdout, "Setting pilot name to \"%s\" ...\n", pilot_name);

    const UTF8ToWideConverter value(pilot_name);
    if (flarm.SetPilot(value, env))
      fprintf(stdout, "Pilot name set to \"%s\"\n", pilot_name);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeCoPilot(FlarmDevice &flarm, OperationEnvironment &env)
{
  while (true) {
    TCHAR old_copilot_name[64];
    if (flarm.GetCoPilot(old_copilot_name, 64, env))
      _tprintf(_T("Old copilot name: \"%s\"\n"), old_copilot_name);

    fprintf(stdout, "Please enter the new copilot name:\n");
    fprintf(stdout, "> ");

    char copilot_name[64];
    if (fgets(copilot_name, 64, stdin) == NULL || strlen(copilot_name) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    StripRight(copilot_name);
    fprintf(stdout, "Setting copilot name to \"%s\" ...\n", copilot_name);

    const UTF8ToWideConverter value(copilot_name);
    if (flarm.SetCoPilot(value, env))
      fprintf(stdout, "CoPilot name set to \"%s\"\n", copilot_name);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangePlaneType(FlarmDevice &flarm, OperationEnvironment &env)
{
  while (true) {
    TCHAR old_plane_type[64];
    if (flarm.GetPlaneType(old_plane_type, 64, env))
      _tprintf(_T("Old plane type: \"%s\"\n"), old_plane_type);

    fprintf(stdout, "Please enter the new plane type:\n");
    fprintf(stdout, "> ");

    char plane_type[64];
    if (fgets(plane_type, 64, stdin) == NULL || strlen(plane_type) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    StripRight(plane_type);
    fprintf(stdout, "Setting plane type to \"%s\" ...\n", plane_type);

    const UTF8ToWideConverter value(plane_type);
    if (flarm.SetPlaneType(value, env))
      fprintf(stdout, "Plane type set to \"%s\"\n", plane_type);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeRegistration(FlarmDevice &flarm, OperationEnvironment &env)
{
  while (true) {
    TCHAR old_registration[64];
    if (flarm.GetPlaneRegistration(old_registration, 64, env))
      _tprintf(_T("Old plane registratio: \"%s\"\n"), old_registration);

    fprintf(stdout, "Please enter the new plane registration:\n");
    fprintf(stdout, "> ");

    char registration[64];
    if (fgets(registration, 64, stdin) == NULL || strlen(registration) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    StripRight(registration);
    fprintf(stdout, "Setting plane registration to \"%s\" ...\n", registration);

    const UTF8ToWideConverter value(registration);
    if (flarm.SetPlaneRegistration(value, env))
      fprintf(stdout, "Plane registration set to \"%s\"\n", registration);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeCompetitionId(FlarmDevice &flarm, OperationEnvironment &env)
{
  while (true) {
    TCHAR old_id[64];
    if (flarm.GetCompetitionId(old_id, 64, env))
      _tprintf(_T("Old competition id: \"%s\"\n"), old_id);

    fprintf(stdout, "Please enter the new competition id:\n");
    fprintf(stdout, "> ");

    char id[64];
    if (fgets(id, 64, stdin) == NULL || strlen(id) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    StripRight(id);
    fprintf(stdout, "Setting competition id to \"%s\" ...\n", id);

    const UTF8ToWideConverter value(id);
    if (flarm.SetCompetitionId(value, env))
      fprintf(stdout, "competition id set to \"%s\"\n", id);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeCompetitionClass(FlarmDevice &flarm, OperationEnvironment &env)
{
  while (true) {
    TCHAR old_comp_class[64];
    if (flarm.GetCompetitionClass(old_comp_class, 64, env))
      _tprintf(_T("Old competition class: \"%s\"\n"), old_comp_class);

    fprintf(stdout, "Please enter the new competition class:\n");
    fprintf(stdout, "> ");

    char comp_class[64];
    if (fgets(comp_class, 64, stdin) == NULL || strlen(comp_class) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    StripRight(comp_class);
    fprintf(stdout, "Setting competition class to \"%s\" ...\n", comp_class);

    const UTF8ToWideConverter value(comp_class);
    if (flarm.SetCompetitionClass(value, env))
      fprintf(stdout, "Competition class set to \"%s\"\n", comp_class);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeRange(FlarmDevice &flarm, OperationEnvironment &env)
{
  while (true) {
    unsigned num_range;

    if (flarm.GetRange(num_range, env))
      printf("Old range setting: \"%d\"\n", num_range);

    fprintf(stdout, "Please enter the range setting (2000-25500):\n");
    fprintf(stdout, "> ");

    char range[64];
    if (fgets(range, 64, stdin) == NULL || strlen(range) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    StripRight(range);

    char *end_ptr;
    num_range = strtoul(range, &end_ptr, 10);
    if (range == end_ptr) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    fprintf(stdout, "Setting range to \"%d\" ...\n", num_range);

    if (flarm.SetRange(num_range, env))
      fprintf(stdout, "Range set to \"%d\"\n", num_range);
    else
      fprintf(stdout, "Operation failed!\n");

    return;
  }
}

static void
ChangeBaudRate(FlarmDevice &flarm, OperationEnvironment &env)
{
  while (true) {
    unsigned baud_id;

    if (flarm.GetBaudRate(baud_id, env))
      printf("Old baud rate setting: \"%d\"\n", baud_id);

    fprintf(stdout, "Please enter the baud rate setting (2000-25500):\n");
    fprintf(stdout, "> ");

    char buffer[64];
    if (fgets(buffer, 64, stdin) == NULL || strlen(buffer) == 0) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    StripRight(buffer);

    char *end_ptr;
    baud_id = strtoul(buffer, &end_ptr, 10);
    if (end_ptr == buffer) {
      fprintf(stdout, "Invalid input\n");
      continue;
    }

    fprintf(stdout, "Setting baud rate to \"%d\" ...\n", baud_id);

    if (flarm.SetBaudRate(baud_id, env))
      fprintf(stdout, "BaudRate set to \"%d\"\n", baud_id);
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
                  "8:  Change baud rate\n"
                  "r:  Restart the FLARM\n"
                  "s+: Enable the stealth mode\n"
                  "s-: Disable the stealth mode\n"
                  "q:  Quit this application\n"
                  "------------------------------------\n");
}

static void
RunUI(FlarmDevice &flarm, OperationEnvironment &env)
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
      ChangePilot(flarm, env);
      break;
    case '2':
      ChangeCoPilot(flarm, env);
      break;
    case '3':
      ChangePlaneType(flarm, env);
      break;
    case '4':
      ChangeRegistration(flarm, env);
      break;
    case '5':
      ChangeCompetitionId(flarm, env);
      break;
    case '6':
      ChangeCompetitionClass(flarm, env);
      break;
    case '7':
      ChangeRange(flarm, env);
      break;
    case '8':
      ChangeBaudRate(flarm, env);
      break;
    case 'r':
    case 'R':
      fprintf(stdout, "Restarting the FLARM ...\n");
      flarm.Restart(env);
      break;
    case 's':
    case 'S':
      if (strlen(in) < 2 || (in[1] != '+' && in[1] != '-')) {
        fprintf(stdout, "Invalid input\n");
        break;
      }

      fprintf(stdout, "Changing stealth mode setting ...\n");
      if (flarm.SetStealthMode(in[1] == '+', env)) {
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

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

int
main(int argc, char **argv)
try {
  Args args(argc, argv, "PORT BAUD");
  DebugPort debug_port(args);
  args.ExpectEnd();

  ScopeGlobalAsioThread global_asio_thread;

  auto port = debug_port.Open(*asio_thread, *(DataHandler *)nullptr);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  FlarmDevice flarm(*port);
  RunUI(flarm, env);

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
