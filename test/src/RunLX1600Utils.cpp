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
#include "Device/Config.hpp"
#include "Device/Driver/LX/LX1600.hpp"
#include "OS/Args.hpp"
#include "Util/StringUtil.hpp"
#include "Util/PrintException.hxx"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "IO/Async/GlobalAsioThread.hpp"
#include "IO/Async/AsioThread.hpp"
#include "Units/System.hpp"
#include "Atmosphere/Pressure.hpp"
#include "IO/DataHandler.hpp"

#include <stdio.h>

static bool
ReadDouble(double &value)
{
  char buffer[64];
  if (fgets(buffer, 64, stdin) == NULL || strlen(buffer) == 0)
    return false;

  char *end_ptr;
  value = strtod(buffer, &end_ptr);
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
ReadDouble(const char *setting, double &value)
{
  PrintInputRequest(setting);
  if (ReadDouble(value))
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
  double mc;
  if (!ReadDouble("the MC setting (0.0 - 5.0)", mc))
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
  double ballast;
  if (!ReadDouble("the Ballast setting (1.0 - 1.5)", ballast))
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
  double altitude_offset;
  if (!ReadDouble("the altitude offset setting (m)", altitude_offset))
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
  double qnh;
  if (!ReadDouble("the QNH setting (hPa)", qnh))
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
SetPolar(Port &port, OperationEnvironment &env)
{
  double a, b, c;
  if (!ReadDouble("polar coefficient a", a) ||
      !ReadDouble("polar coefficient b", b) ||
      !ReadDouble("polar coefficient c", c))
    return;

  if (LX1600::SetPolar(port, env, a, b, c))
    fprintf(stdout, "Polar coefficients set to \"%.2f, %.2f, %.2f\"\n",
            (double)a, (double)b, (double)c);
  else
    fprintf(stdout, "Operation failed!\n");
}

static void
SetFilters(Port &port, OperationEnvironment &env)
{
  double vario_filter, te_filter;
  unsigned te_level;
  if (!ReadDouble("the Vario filter (sec, default = 1)", vario_filter) ||
      !ReadDouble("the TE filter (0.1 - 2.0, default = 1.5)", te_filter) ||
      !ReadUnsigned("the TE level (50 - 150 %, default = 0 = off)", te_level))
    return;

  if (LX1600::SetFilters(port, env, vario_filter, te_filter, te_level))
    fprintf(stdout, "Filters set to \"%.1f, %.1f, %u\"\n",
            (double)vario_filter, (double)te_filter, te_level);
  else
    fprintf(stdout, "Operation failed!\n");
}

static void
SetSCSettings(Port &port, OperationEnvironment &env)
{
  unsigned mode, control_mode;
  double deadband, threshold_speed;

  if (!ReadUnsigned("the SC Mode (EXTERNAL = 0, default = ON_CIRCLING = 1, AUTO_IAS = 2)", mode) ||
      !ReadDouble("the SC deadband (0 - 10.0 m/s, default=1)", deadband) ||
      !ReadUnsigned("the SC switch mode (NORMAL = 0, default = INVERTED = 1, TASTER = 2)", control_mode))
    return;

  if (mode != 2u)
    threshold_speed = 0;
  else if (!ReadDouble("the SC threshold speed (50 - 150 km/h, default=110)", threshold_speed))
    return;

  if (LX1600::SetSCSettings(port, env, (LX1600::SCMode)mode, deadband,
                            (LX1600::SCControlMode)control_mode, threshold_speed))
    fprintf(stdout, "SC settings changed!\n");
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
                  "p:  Set polar coefficients\n"
                  "f:  Set filter settings\n"
                  "s:  Set speed control settings\n"
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
    case 'p':
    case 'P':
      SetPolar(port, env);
      break;
    case 'f':
    case 'F':
      SetFilters(port, env);
      break;
    case 's':
    case 'S':
      SetSCSettings(port, env);
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

  NullDataHandler handler;
  auto port = debug_port.Open(*asio_thread, *(DataHandler *)nullptr);

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  RunUI(*port, env);

  return EXIT_SUCCESS;
} catch (const std::exception &exception) {
  PrintException(exception);
  return EXIT_FAILURE;
}
