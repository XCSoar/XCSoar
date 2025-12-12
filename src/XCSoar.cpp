// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * This is the main entry point for the application
 * @file XCSoar.cpp
 */

#include "Startup.hpp"
#include "LocalPath.hpp"
#include "Version.hpp"
#include "ProductName.hpp"
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
#include "Profile/Profile.hpp"
#include "Profile/Map.hpp"
#include "Profile/Current.hpp"

#ifdef ENABLE_SDL
/* this is necessary on macOS, to let libSDL bootstrap Quartz
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
  "  -version       display version information and exit\n"
  "  -datapath=      path to " PRODUCT_NAME_A " data can be defined\n"
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
  // Load anti-aliasing setting early before initializing the display
  unsigned antialiasing_samples = 0;
  try {
    if (Profile::GetPath() == nullptr)
      Profile::SetFiles(nullptr);
    
    Profile::LoadFile(Profile::GetPath());
    Profile::map.Get(ProfileKeys::AntiAliasing, antialiasing_samples);
    if (antialiasing_samples != 0 && antialiasing_samples != 2 &&
        antialiasing_samples != 4 && antialiasing_samples != 8 &&
        antialiasing_samples != 16)
      antialiasing_samples = 0;
  } catch (...) {
    // Ignore errors loading profile at this stage
  }

  ScreenGlobalInit screen_init(antialiasing_samples);

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

  // Read options from the command line
  {
#ifdef _WIN32
    Args args(GetCommandLine(), Usage);
#else
    Args args(argc, argv, Usage);
#endif
    CommandLine::Parse(args);
  }

  InitialiseDataPath();

  // Write startup note + version to logfile
  LogFormat(_T("Starting %s"), XCSoar_ProductToken);

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
