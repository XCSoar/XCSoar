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
#include "Look/GlobalFonts.hpp"
#include "ui/window/Init.hpp"
#include "net/http/Init.hpp"
#include "ResourceLoader.hpp"
#include "Language/Language.hpp"
#include "Language/LanguageGlue.hpp"
#include "Simulator.hpp"
#include "Audio/GlobalPCMMixer.hpp"
#include "Audio/GlobalPCMResourcePlayer.hpp"
#include "Audio/GlobalVolumeController.hpp"
#include "system/Args.hpp"
#include "io/async/GlobalAsioThread.hpp"
#include "io/async/AsioThread.hpp"
#include "util/PrintException.hxx"

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

#include <cassert>

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
#ifdef _WIN32
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

#ifdef _WIN32
  /* try to make the UI most responsive */
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif

  AllowLanguage();
  InitLanguage();

  ScopeGlobalAsioThread global_asio_thread;
  const Net::ScopeInit net_init(asio_thread->GetEventLoop());

  ScopeGlobalPCMMixer global_pcm_mixer(asio_thread->GetEventLoop());
  ScopeGlobalPCMResourcePlayer global_pcm_resouce_player;
  ScopeGlobalVolumeController global_volume_controller;

  // Perform application initialization and run loop
  int ret = EXIT_FAILURE;
  if (Startup(screen_init.GetDisplay()))
    ret = CommonInterface::main_window->RunEventLoop();

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
WinMain(HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance,
        [[maybe_unused]] LPSTR lpCmdLine2,
        [[maybe_unused]] int nCmdShow)
#endif
try {
#ifdef USE_WIN32_RESOURCES
  ResourceLoader::Init(hInstance);
#endif

  InitialiseDataPath();

  // Write startup note + version to logfile
  LogFormat(_T("Starting %s"), XCSoar_ProductToken);

  // Read options from the command line
  {
#ifdef _WIN32
    Args args(GetCommandLine(), Usage);
#else
    Args args(argc, argv, Usage);
#endif
    CommandLine::Parse(args);
  }

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
