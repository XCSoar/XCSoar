// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/ContainerWindow.hpp"
#include "ui/control/ProgressBar.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Color.hpp"
#include "Gauge/LogoView.hpp"
#include "util/StaticString.hxx"

#include <chrono>

/**
 * The XCSoar splash screen with a progress bar.
 */
class ProgressWindow : public ContainerWindow {
  Color background_color;
  bool dark_mode;

  Bitmap bitmap_progress_border;

  Font font;

  LogoView logo;

  StaticString<512> message;

  ProgressBar progress_bar;

  unsigned text_height;

  /** Lines reserved above the bar.  At least two, so the splash
      layout stays put when the message is a single line. */
  unsigned message_lines = 2;

  unsigned range_min = 0, range_max = 0;
  std::chrono::steady_clock::time_point rate_start{};
  unsigned rate_done = 0;
  bool rate_started = false;
  /** Smoothed remaining seconds.  Negative until a sample exists. */
  int shown_secs = -1;

  /** Bytes received, when the caller reports them.  The bar position
      stays in the caller's own units. */
  unsigned byte_count = 0;
  bool have_bytes = false;
  /** Baseline of the speed sample (bytes, or bar units). */
  unsigned speed_done = 0;
  /** Smoothed units per second.  Negative until a sample exists. */
  int shown_rate = -1;

  PixelRect logo_position, message_position;
  PixelRect bottom_position, progress_bar_position;

public:
  explicit ProgressWindow(ContainerWindow &parent) noexcept;

  void SetMessage(const char *text) noexcept;

  void SetRange(unsigned min_value, unsigned max_value) noexcept;
  void SetByteCount(unsigned bytes) noexcept;
  void SetStep(unsigned size) noexcept;
  void SetValue(unsigned value) noexcept;
  void Step() noexcept;

private:
  void UpdateLayout(PixelRect rc) noexcept;
  void ResetRate() noexcept;
  void UpdateRate(unsigned done, unsigned total) noexcept;
  void UpdateBarLabel(unsigned value) noexcept;

protected:
  void OnResize(PixelSize new_size) noexcept override;
  void OnPaint(Canvas &canvas) noexcept override;
};
