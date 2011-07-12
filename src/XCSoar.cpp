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
#include "Screen/Fonts.hpp"
#include "Screen/Init.hpp"
#include "Screen/Graphics.hpp"
#include "UtilsSystem.hpp"

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
  InitialiseDataPath();
  StartupLogFreeRamAndStorage();

  // Write startup note + version to logfile
  LogStartUp(_T("Starting XCSoar %s"), XCSoar_VersionString);

  // Read options from the command line
#ifndef WIN32
  HINSTANCE hInstance = NULL;
  const TCHAR *lpCmdLine = argc >= 2 ? argv[1] : _T("");
#elif !defined(_WIN32_WCE)
  /* on Windows (non-CE), the lpCmdLine argument is narrow, and we
     have to use GetCommandLine() to get the UNICODE string */
  LPCTSTR lpCmdLine = GetCommandLine();
#endif

  ParseCommandLine(lpCmdLine);

  ScreenGlobalInit screen_init;

  // Write initialization note to logfile
  LogStartUp(_T("Initialise application instance"));

#ifdef WIN32
  /* try to make the UI most responsive */
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

  // Perform application initialization and run loop
  int ret = EXIT_FAILURE;
  if (XCSoarInterface::Startup(hInstance))
    ret = CommonInterface::main_window.event_loop();

  CommonInterface::main_window.reset();

  Fonts::Deinitialize();
  Graphics::Deinitialise();

  DeinitialiseDataPath();

  return ret;
}
