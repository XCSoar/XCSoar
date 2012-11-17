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

#include "CommandLine.hpp"
#include "Profile/Profile.hpp"
#include "OS/Args.hpp"
#include "OS/PathName.hpp"
#include "Hardware/Display.hpp"
#include "Simulator.hpp"

#ifdef WIN32
#include <windows.h> /* for AllocConsole() */
#endif

namespace CommandLine {
#if !defined(_WIN32_WCE)
  unsigned width = 640, height = 480;
#endif

#ifdef HAVE_CMDLINE_FULLSCREEN
  bool full_screen = false;
#endif

#ifdef HAVE_CMDLINE_RESIZABLE
  bool resizable = false;
#endif
}

void
CommandLine::Parse(Args &args)
{
  while (!args.IsEmpty()) {
    const char *s = args.GetNext();

    if (*s != '-') {
#ifdef _WIN32
      continue;
#else
      args.UsageError();
#endif
    }

    // Also accept "--" prefix for arguments. Usually used on UNIX for long options
    if (s[1] == '-')
      s++;

    if (strncmp(s, "-profile=", 9) == 0) {
      s += 9;
      PathName convert(s);
      Profile::SetFiles(convert);
    }
#ifdef SIMULATOR_AVAILABLE
    else if (strcmp(s, "-simulator") == 0) {
      global_simulator_flag = true;
      sim_set_in_cmd_line_flag = true;
    }
    else if (strcmp(s, "-fly") == 0) {
      global_simulator_flag=false;
      sim_set_in_cmd_line_flag=true;
    }
#endif
#if !defined(_WIN32_WCE)
    else if (isdigit(s[1])) {
      char *p;
      width = strtol(s+1, &p, 10);
      if (*p != 'x' && *p != 'X')
        args.UsageError();
      s = p;
      height = strtol(s+1, &p, 10);
    }
    else if (strcmp(s, "-portrait") == 0) {
      width = 480;
      height = 640;
    }
    else if (strcmp(s, "-square") == 0) {
      width = 480;
      height = 480;
    }
    else if (strcmp(s, "-small") == 0) {
      width = 320;
      height = 240;
    }
#endif
#ifdef HAVE_CMDLINE_FULLSCREEN
    else if (strcmp(s, "-fullscreen") == 0) {
      full_screen = true;
    }
#endif
#ifdef HAVE_CMDLINE_RESIZABLE
    else if (strcmp(s, "-resizable") == 0) {
      resizable = true;
    }
#endif
#if defined(_WIN32) && !defined(_WIN32_WCE) && !defined(__WINE__)
    else if (strcmp(s, "-console") == 0) {
      AllocConsole();
      freopen("CONOUT$", "wb", stdout);
    }
#endif
#if !defined(ANDROID) && !defined(_WIN32_WCE)
    else if (strncmp(s, "-dpi=", 5) == 0) {
      unsigned x_dpi, y_dpi;
      char *p;
      x_dpi = strtol(s+5, &p, 10);
      if (*p == 'x' || *p == 'X') {
        s = p;
        y_dpi = strtol(s+1, &p, 10);
      } else
        y_dpi = x_dpi;

      if (x_dpi < 32 || x_dpi > 512 || y_dpi < 32 || y_dpi > 512)
        args.UsageError();

      Display::SetDPI(x_dpi, y_dpi);
    }
#endif
#ifndef _WIN32
    else
      args.UsageError();
#endif
  }

#if !defined(_WIN32_WCE)
  if (width < 240 || width > 4096 ||
      height < 240 || height > 4096)
    args.UsageError();
#endif
}
