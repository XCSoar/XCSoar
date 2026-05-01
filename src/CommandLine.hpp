// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <cstdint>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

class Args;

namespace CommandLine {

  /**
   * If Auto, @ref HasTouchScreen follows device detection.  Force/Disable
   * come from the @c -touchscreen and @c -notouchscreen switches; if both
   * are given, the last one wins.
   */
  enum class TouchInput : uint8_t { Auto, Force, Disable };
  inline TouchInput touch_input = TouchInput::Auto;

  /**
   * Apply the command-line @ref touch_input override to a hardware
   * detection value.
   */
  inline bool
  ApplyTouchInputOverride(bool from_hardware) noexcept
  {
    switch (touch_input) {
    case TouchInput::Force:
      return true;
    case TouchInput::Disable:
      return false;
    case TouchInput::Auto:
    default:
      return from_hardware;
    }
  }

  extern unsigned width, height;

#ifdef KOBO
  static constexpr bool full_screen = false;
#elif defined(ENABLE_SDL) || defined(USE_X11)
#define HAVE_CMDLINE_FULLSCREEN
  extern bool full_screen;
#else
  static constexpr bool full_screen = false;
#endif

#if (defined(__linux__) && !defined(ANDROID)) || \
    (defined(__APPLE__) && !TARGET_OS_IPHONE)
#define HAVE_CMDLINE_REPLAY
  extern const char *replay_path;
#endif

/**
 * Multi-line option text for Args (printed by UsageError on stderr);
 * begins with a newline.
 */
[[nodiscard]] const char *
OptionSummary() noexcept;

/**
 * Print full option summary to standard output (--help), then the caller
 * should exit successfully.
 */
void
PrintHelp() noexcept;

/**
 * Reads and parses arguments/options from the command line
 * @param CommandLine command line argument string
 */
  void Parse(Args &args);
}
