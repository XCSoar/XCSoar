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

#ifndef XCSOAR_OS_COMMAND_LINE_HPP
#define XCSOAR_OS_COMMAND_LINE_HPP

class Args;

namespace CommandLine {
  extern unsigned width, height;

#ifdef KOBO
  static constexpr bool full_screen = false;
#elif defined(ENABLE_SDL) || defined(USE_X11)
#define HAVE_CMDLINE_FULLSCREEN
  extern bool full_screen;
#else
  static constexpr bool full_screen = false;
#endif

#if defined(__linux__) && !defined(ANDROID)
#define HAVE_CMDLINE_REPLAY
  extern const char *replay_path;
#endif

/**
 * Reads and parses arguments/options from the command line
 * @param CommandLine command line argument string
 */
  void Parse(Args &args);
}

#endif
