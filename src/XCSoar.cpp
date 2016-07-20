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

/**
 * This is the main entry point for the application
 * @file XCSoar.cpp
 */

#include "Startup.hpp"
#include "LocalPath.hpp"
#include "Version.hpp"
#include "LogFile.hpp"
#include "CommandLine.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Look/GlobalFonts.hpp"
#include "Screen/Init.hpp"
#include "Net/HTTP/Init.hpp"
#include "UtilsSystem.hpp"
#include "ResourceLoader.hpp"
#include "Language/Language.hpp"
#include "Language/LanguageGlue.hpp"
#include "Simulator.hpp"
#include "Audio/GlobalPCMMixer.hpp"
#include "Audio/GlobalPCMResourcePlayer.hpp"
#include "Audio/GlobalVolumeController.hpp"
#include "OS/Args.hpp"
#include "IO/Async/GlobalAsioThread.hpp"

#ifndef NDEBUG
#include "Thread/Thread.hpp"
#endif

#ifdef ENABLE_SDL
/* this is necessary on Mac OS X, to let libSDL bootstrap Quartz
   before entering our main() */
#include <SDL_main.h>
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE
#import <AppKit/AppKit.h>
#endif
#endif

#include <assert.h>

static const char *const Usage = "\n"
  "  -datapath=      path to XCSoar data can be defined\n"
#ifdef SIMULATOR_AVAILABLE
  "  -simulator      bypass startup-screen, use simulator mode directly\n"
  "  -fly            bypass startup-screen, use fly mode directly\n"
#endif
  "  -profile=fname  load profile from file fname\n"
  "  -WIDTHxHEIGHT   use screen resolution WIDTH x HEIGHT\n"
  "  -portrait       use a 480x640 screen resolution\n"
  "  -square         use a 480x480 screen resolution\n"
  "  -small          use a 320x240 screen resolution\n"
#if !defined(ANDROID)
  "  -dpi=DPI        force usage of DPI for pixel density\n"
  "  -dpi=XDPIxYDPI  force usage of XDPI and YDPI for pixel density\n"
#endif
#ifdef HAVE_CMDLINE_FULLSCREEN
  "  -fullscreen     full-screen mode\n"
#endif
#ifdef HAVE_CMDLINE_RESIZABLE
  "  -resizable      resizable window\n"
#endif
#ifdef WIN32
  "  -console        open debug output console\n"
#endif
  ;

static int
Main()
{
  ScreenGlobalInit screen_init;

#if defined(__APPLE__) && !TARGET_OS_IPHONE
  // We do not want the ugly non-localized main menu which SDL creates
  [NSApp setMainMenu: [[NSMenu alloc] init]];
#endif

#ifdef WIN32
  /* try to make the UI most responsive */
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

  AllowLanguage();
  InitLanguage();

  ScopeGlobalAsioThread global_asio_thread;

  ScopeGlobalPCMMixer global_pcm_mixer;
  ScopeGlobalPCMResourcePlayer global_pcm_resouce_player;
  ScopeGlobalVolumeController global_volume_controller;

  // Perform application initialization and run loop
  int ret = EXIT_FAILURE;
  if (Startup())
    ret = CommonInterface::main_window->RunEventLoop();

  Shutdown();

  DisallowLanguage();

  Fonts::Deinitialize();

  DeinitialiseDataPath();
  Net::Deinitialise();

  return ret;
}

/**
 * Main entry point for the whole XCSoar application
 */
#ifndef WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        gcc_unused LPSTR lpCmdLine2,
        int nCmdShow)
#endif
{
#ifdef USE_WIN32_RESOURCES
  ResourceLoader::Init(hInstance);
#endif

  Net::Initialise();

  InitialiseDataPath();
  StartupLogFreeRamAndStorage();

  // Write startup note + version to logfile
  LogFormat(_T("Starting XCSoar %s"), XCSoar_ProductToken);

  // Read options from the command line
  {
#ifdef WIN32
    Args args(GetCommandLine(), Usage);
#else
    Args args(argc, argv, Usage);
#endif
    CommandLine::Parse(args);
  }

  int ret = Main();

  assert(!ExistsAnyThread());

#if defined(__APPLE__) && TARGET_OS_IPHONE
  /* For some reason, the app process does not exit on iOS, but a black
   * screen remains, if the process is not explicitly terminated */
  exit(ret);
#endif

  return ret;
}
