// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "CommandLine.hpp"
#include "Profile/Profile.hpp"
#include "system/Args.hpp"
#include "system/ConvertPathName.hpp"
#include "Hardware/DisplayDPI.hpp"
#include "Simulator.hpp"
#include "LocalPath.hpp"
#include "util/StringCompare.hxx"
#include "util/StringAPI.hxx"
#include "util/NumberParser.hpp"
#include "Asset.hpp"

#ifdef _WIN32
#include <windows.h> /* for AllocConsole() */
#endif

namespace CommandLine {
  unsigned width = IsKobo() ? 600 : 640;
  unsigned height = IsKobo() ? 800 : 480;

#ifdef HAVE_CMDLINE_FULLSCREEN
  bool full_screen = false;
#endif

#ifdef HAVE_CMDLINE_REPLAY
  const char *replay_path;
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

    if (StringIsEqual(s, "-profile=", 9)) {
      s += 9;

      if (StringIsEmpty(s))
        args.UsageError();

      PathName convert(s);
      Profile::SetFiles(convert);
    } else if (StringIsEqual(s, "-datapath=", 10)) {
      s += 10;
      PathName convert(s);
      SetSingleDataPath(convert);
#ifdef HAVE_CMDLINE_REPLAY
    } else if (StringIsEqual(s, "-replay=", 8)) {
      replay_path = s + 8;
#endif
#ifdef SIMULATOR_AVAILABLE
    } else if (StringIsEqual(s, "-simulator")) {
      global_simulator_flag = true;
      sim_set_in_cmd_line_flag = true;
    } else if (StringIsEqual(s, "-fly")) {
      global_simulator_flag=false;
      sim_set_in_cmd_line_flag=true;
#endif
    } else if (isdigit(s[1])) {
      char *p;
      width = ParseUnsigned(s + 1, &p);
      if (*p != 'x' && *p != 'X')
        args.UsageError();
      s = p;
      height = ParseUnsigned(s + 1, &p);
      if (*p != '\0')
        args.UsageError();
    } else if (StringIsEqual(s, "-portrait")) {
      width = 480;
      height = 640;
    } else if (StringIsEqual(s, "-square")) {
      width = 480;
      height = 480;
    } else if (StringIsEqual(s, "-small")) {
      width = 320;
      height = 240;
#ifdef HAVE_CMDLINE_FULLSCREEN
    } else if (StringIsEqual(s, "-fullscreen")) {
      full_screen = true;
#endif
#ifdef _WIN32
    } else if (StringIsEqual(s, "-console")) {
      AllocConsole();
      freopen("CONOUT$", "wb", stdout);
#endif
#if !defined(ANDROID)
    } else if (StringIsEqual(s, "-dpi=", 5)) {
      unsigned x_dpi, y_dpi;
      char *p;
      x_dpi = ParseUnsigned(s + 5, &p);
      if (*p == 'x' || *p == 'X') {
        s = p;
        y_dpi = ParseUnsigned(s + 1, &p);
      } else
        y_dpi = x_dpi;
      if (*p != '\0')
        args.UsageError();

      if (x_dpi < 32 || x_dpi > 512 || y_dpi < 32 || y_dpi > 512)
        args.UsageError();

      Display::SetForcedDPI(x_dpi, y_dpi);
#endif
#ifdef __APPLE__
    } else if (StringStartsWith(s, "-psn")) {
      /* The OS X launcher always supplies some process number argument.
         Just ignore it.
      */
#endif
    } else {
#ifndef _WIN32
      args.UsageError();
#endif
    }
  }

  if (width < 240 || width > 4096 ||
      height < 240 || height > 4096)
    args.UsageError();
}
