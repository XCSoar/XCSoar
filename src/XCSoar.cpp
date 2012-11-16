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

/**
 * This is the main entry point for the application
 * @file XCSoar.cpp
 */

#include "resource.h"
#include "LocalPath.hpp"
#include "Version.hpp"
#include "Protection.hpp"
#include "Components.hpp"
#include "LogFile.hpp"
#include "CommandLine.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Compiler.h"
#include "Look/Fonts.hpp"
#include "Screen/Init.hpp"
#include "Net/Init.hpp"
#include "UtilsSystem.hpp"
#include "ResourceLoader.hpp"
#include "Language/Language.hpp"
#include "Simulator.hpp"
#include "OS/Args.hpp"
#include "IO/Async/GlobalIOThread.hpp"

#ifndef NDEBUG
#include "Thread/Thread.hpp"
#endif

#include <assert.h>

static const char *Usage = "\n"
#ifdef SIMULATOR_AVAILABLE
  "  -simulator      bypass startup-screen, use simulator mode directly\n"
  "  -fly            bypass startup-screen, use fly mode directly\n"
#endif
  "  -profile=fname  load profile from file fname\n"
#if !defined(_WIN32_WCE)
  "  -WIDTHxHEIGHT   use screen resolution WIDTH x HEIGHT\n"
  "  -portrait       use a 480x640 screen resolution\n"
  "  -square         use a 480x480 screen resolution\n"
  "  -small          use a 320x240 screen resolution\n"
#endif
#if !defined(ANDROID) && !defined(_WIN32_WCE)
  "  -dpi=DPI        force usage of DPI for pixel density\n"
  "  -dpi=XDPIxYDPI  force usage of XDPI and YDPI for pixel density\n"
#endif
#ifdef HAVE_CMDLINE_FULLSCREEN
  "  -fullscreen     full-screen mode\n"
#endif
#ifdef HAVE_CMDLINE_RESIZABLE
  "  -resizable      resizable window\n"
#endif
#if defined(_WIN32) && !defined(_WIN32_WCE)&& !defined(__WINE__)
  "  -console        open debug output console\n"
#endif
  ;

/**
 * Main entry point for the whole XCSoar application
 */
#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
#ifdef _WIN32_WCE
        gcc_unused LPWSTR lpCmdLine,
#else
        gcc_unused LPSTR lpCmdLine2,
#endif
        int nCmdShow)
#endif
{
#ifdef WIN32
  ResourceLoader::Init(hInstance);
#endif

  Net::Initialise();

  InitialiseDataPath();
  StartupLogFreeRamAndStorage();

  // Write startup note + version to logfile
  LogStartUp(_T("Starting XCSoar %s"), XCSoar_ProductToken);

  // Read options from the command line
#ifndef WIN32
  CommandLine::Parse(Args(argc, argv, Usage));
#else
  CommandLine::Parse(Args(GetCommandLine(), Usage));
#endif

  ScreenGlobalInit screen_init;

#ifdef WIN32
  /* try to make the UI most responsive */
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

  AllowLanguage();

  InitialiseIOThread();

  // Perform application initialization and run loop
  int ret = EXIT_FAILURE;
  if (XCSoarInterface::Startup())
    ret = CommonInterface::main_window->RunEventLoop();

  delete CommonInterface::main_window;

  DeinitialiseIOThread();

  DisallowLanguage();

  Fonts::Deinitialize();

  DeinitialiseDataPath();
  Net::Deinitialise();

  assert(!ExistsAnyThread());

  return ret;
}
