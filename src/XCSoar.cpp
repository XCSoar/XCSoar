// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
#include "UISettings.hpp"
#include "Look/GlobalFonts.hpp"
#include "ui/window/Init.hpp"
#include "net/http/Init.hpp"
#include "Language/Language.hpp"
#include "Language/LanguageGlue.hpp"
#include "Simulator.hpp"
#include "Audio/GlobalPCMMixer.hpp"
#include "Audio/GlobalPCMResourcePlayer.hpp"
#include "Audio/GlobalVolumeController.hpp"
#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "system/Args.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "util/PrintException.hxx"
#include "Profile/Profile.hpp"
#include "Profile/File.hpp"
#include "Profile/Map.hpp"
#include "Profile/Keys.hpp"

#ifdef ENABLE_SDL
#ifdef SDL_MAIN_HANDLED
/* When SDL_MAIN_HANDLED is defined, we must call SDL_SetMainReady()
   before using SDL to avoid SDL's -Dmain=SDL_main
   macro which conflicts with "main" in the XCSoar code */
#include <SDL.h>
#else
/* this is necessary on macOS, to let libSDL bootstrap Quartz
   before entering our main() */
#include <SDL_main.h>
#endif
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#if !TARGET_OS_IPHONE
#include "Apple/MacOSMainMenu.hpp"
#endif
#endif

#include <cassert>

static int
Main()
{
  // Peek at the profile for the AA setting without modifying global
  // Profile state - Profile::GetPath() must remain nullptr so that
  // dlgStartupShowModal() is still shown later.  Unless a profile was
  // given on the command line, use the most recently used one: that is
  // what the startup dialog will preselect.
  unsigned antialiasing_samples = ANTIALIASING_OFF;
  try {
    Path path = Profile::GetPath();
    AllocatedPath default_path;
    if (path == nullptr) {
      default_path = Profile::GetMostRecentPath();
      path = default_path;
    }
    ProfileMap temp_map;
    Profile::LoadFile(temp_map, path);
    temp_map.Get(ProfileKeys::AntiAliasing, antialiasing_samples);
    if (!IsValidAntialiasing(antialiasing_samples))
      antialiasing_samples = ANTIALIASING_OFF;
  } catch (...) {
  }

  ScreenGlobalInit screen_init(antialiasing_samples);

#ifdef _WIN32
  /* try to make the UI most responsive */
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

  AllowLanguage();
  InitLanguage();

#if defined(__APPLE__) && !TARGET_OS_IPHONE
  InitialiseMacOSMainMenu();
#endif

  ScopeGlobalAsioThread global_asio_thread;
  const Net::ScopeInit net_init(asio_thread->GetEventLoop());

  ScopeGlobalPCMMixer global_pcm_mixer(asio_thread->GetEventLoop());
  ScopeGlobalPCMResourcePlayer global_pcm_resouce_player;
  ScopeGlobalVolumeController global_volume_controller;

  // Perform application initialization and run loop
  int ret = EXIT_FAILURE;
  if (Startup(screen_init.GetDisplay()))
    ret = CommonInterface::main_window->RunEventLoop();
  else if (WasStartupCancelledByUser())
    /* quitting from the startup dialogs is a deliberate user action,
       not an error */
    ret = EXIT_SUCCESS;

  /* The export-flight cache owns an InjectTask on the Asio event loop. */
  ShutdownExportFlightsPanel();

  Shutdown();

  DisallowLanguage();

  Fonts::Deinitialize();

  DeinitialiseDataPath();

  return ret;
}

/**
 * Main entry point for the whole XCSoar application
 */
#ifndef _WIN32
int main(int argc, char **argv)
#else
int WINAPI
WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance,
        [[maybe_unused]] LPSTR lpCmdLine2,
        [[maybe_unused]] int nCmdShow)
#endif
try {
#if defined(ENABLE_SDL) && defined(SDL_MAIN_HANDLED)
  SDL_SetMainReady();
#endif

  // Read options from the command line
  {
#ifdef _WIN32
    Args args(GetCommandLine(), CommandLine::OptionSummary());
#else
    Args args(argc, argv, CommandLine::OptionSummary());
#endif
    CommandLine::Parse(args);
  }

  InitialiseDataPath();
  CommandLine::ApplyPendingProfile();

  // Write startup note + version to logfile
  LogFormat("Starting %s", XCSoar_ProductToken);

  int ret = Main();

#if defined(__APPLE__) && TARGET_OS_IPHONE
  /* For some reason, the app process does not exit on iOS, but a black
   * screen remains, if the process is not explicitly terminated */
  exit(ret);
#endif

  return ret;
} catch (...) {
  PrintException(std::current_exception());
  return EXIT_FAILURE;
}
