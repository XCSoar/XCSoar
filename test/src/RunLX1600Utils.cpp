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

#include "DebugPort.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Driver.hpp"
#include "Device/Register.hpp"
#include "Device/Parser.hpp"
#include "Device/Driver/LX/LX1600.hpp"
#include "OS/PathName.hpp"
#include "OS/Args.hpp"
#include "Profile/DeviceConfig.hpp"
#include "Util/StringUtil.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "IO/Async/GlobalIOThread.hpp"
#include "Units/System.hpp"
#include "Atmosphere/Pressure.hpp"
#include "IO/DataHandler.hpp"

#include <stdio.h>

static bool
ReadFixed(fixed &value)
{
  char buffer[64];
  if (fgets(buffer, 64, stdin) == NULL || strlen(buffer) == 0)
    return false;

  char *end_ptr;
  value = fixed(strtod(buffer, &end_ptr));
  return end_ptr != buffer;
}

static bool
ReadUnsigned(unsigned &value)
{
  char buffer[64];
  if (fgets(buffer, 64, stdin) == NULL || strlen(buffer) == 0)
    return false;

  char *end_ptr;
  value = strtoul(buffer, &end_ptr, 10);
  return end_ptr != buffer;
}

static void
PrintInputRequest(const char *setting)
{
  fprintf(stdout, "Please enter %s:\n", setting);
  fprintf(stdout, "> ");
}

static bool
ReadFixed(const char *setting, fixed &value)
{
  PrintInputRequest(setting);
  if (ReadFixed(value))
    return true;

  fprintf(stdout, "Invalid input\n");
  return false;
}

static bool
ReadUnsigned(const char *setting, unsigned &value)
{
  PrintInputRequest(setting);
  if (ReadUnsigned(value))
    return true;

  fprintf(stdout, "Invalid input\n");
  return false;
}

static void
SetMC(Port &port, OperationEnvironment &env)
{
  fixed mc;
  if (!ReadFixed("the MC setting (0.0 - 5.0)", mc))
    return;

  fprintf(stdout, "Setting MC to \"%.1f\" ...\n", (double)mc);

  if (LX1600::SetMacCready(port, env, mc))
    fprintf(stdout, "MC set to \"%.1f\"\n", (double)mc);
  else
    fprintf(stdout, "Operation failed!\n");
}

static void
SetBallast(Port &port, OperationEnvironment &env)
{
  fixed ballast;
  if (!ReadFixed("the Ballast setting (1.0 - 1.5)", ballast))
    return;

  fprintf(stdout, "Setting Ballast to \"%.1f\" ...\n", (double)ballast);

  if (LX1600::SetBallast(port, env, ballast))
    fprintf(stdout, "Ballast set to \"%.1f\"\n", (double)ballast);
  else
    fprintf(stdout, "Operation failed!\n");
}

static void
SetBugs(Port &port, OperationEnvironment &env)
{
  unsigned bugs;
  if (!ReadUnsigned("the Bugs setting (0 - 30%)", bugs))
    return;

  fprintf(stdout, "Setting Bugs to \"%u\" ...\n", bugs);

  if (LX1600::SetBugs(port, env, bugs))
    fprintf(stdout, "Bugs set to \"%u\"\n", bugs);
  else
    fprintf(stdout, "Operation failed!\n");
}

static void
SetAltitudeOffset(Port &port, OperationEnvironment &env)
{
  fixed altitude_offset;
  if (!ReadFixed("the altitude offset setting (m)", altitude_offset))
    return;

  fprintf(stdout, "Setting altitude offset to \"%.1f m\" ...\n",
          (double)altitude_offset);

  if (LX1600::SetAltitudeOffset(port, env, Units::ToUserUnit(altitude_offset, Unit::FEET)))
    fprintf(stdout, "Altitude offset set to \"%.1f m\"\n",
            (double)altitude_offset);
  else
    fprintf(stdout, "Operation failed!\n");
}

static void
SetQNH(Port &port, OperationEnvironment &env)
{
  fixed qnh;
  if (!ReadFixed("the QNH setting (hPa)", qnh))
    return;

  fprintf(stdout, "Setting QNH to \"%.1f hPa\" ...\n",
          (double)qnh);

  if (LX1600::SetQNH(port, env, AtmosphericPressure::HectoPascal(qnh)))
    fprintf(stdout, "QNH set to \"%.1f hPa\"\n",
            (double)qnh);
  else
    fprintf(stdout, "Operation failed!\n");
}

static void
SetVolume(Port &port, OperationEnvironment &env)
{
  unsigned volume;
  if (!ReadUnsigned("the Volume setting (0 - 100%)", volume))
    return;

  fprintf(stdout, "Setting Volume to \"%u %%\" ...\n", volume);

  if (LX1600::SetVolume(port, env, volume))
    fprintf(stdout, "Volume set to \"%u %%\"\n", volume);
  else
    fprintf(stdout, "Operation failed!\n");
}

static void
WriteMenu()
{
  fprintf(stdout, "------------------------------------\n"
                  "LX1600 Utils Menu\n"
                  "------------------------------------\n"
                  "Press any of the following commands:\n\n"
                  "h:  Display this menu\n"
                  "1:  Set the MC\n"
                  "2:  Set Ballast\n"
                  "3:  Set Bugs\n"
                  "4:  Set Altitude Offset\n"
                  "5:  Set QNH\n"
                  "6:  Set Volume\n"
                  "q:  Quit this application\n"
                  "------------------------------------\n");
}

static void
RunUI(Port &port, OperationEnvironment &env)
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
      SetMC(port, env);
      break;
    case '2':
      SetBallast(port, env);
      break;
    case '3':
      SetBugs(port, env);
      break;
    case '4':
      SetAltitudeOffset(port, env);
      break;
    case '5':
      SetQNH(port, env);
      break;
    case '6':
      SetVolume(port, env);
      break;
    case 'q':
    case 'Q':
      fprintf(stdout, "Closing LX1600 Utils ...\n");
      return;
    default:
      fprintf(stdout, "Invalid input\n");
      break;
    }
  }
}

class NullDataHandler : public DataHandler {
public:
  virtual void DataReceived(const void *data, size_t length) {}
};

int
main(int argc, char **argv)
{
  Args args(argc, argv, "PORT BAUD");
  const DeviceConfig config = ParsePortArgs(args);
  args.ExpectEnd();

  InitialiseIOThread();

  NullDataHandler handler;
  std::unique_ptr<Port> port(OpenPort(config, handler));
  if (!port) {
    fprintf(stderr, "Failed to open COM port\n");
    return EXIT_FAILURE;
  }

  ConsoleOperationEnvironment env;
  RunUI(*port, env);

  DeinitialiseIOThread();

  return EXIT_SUCCESS;
}
