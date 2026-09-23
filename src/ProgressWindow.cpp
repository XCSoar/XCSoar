// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProgressWindow.hpp"
#include "GlobalSettings.hpp"
#include "Look/Colors.hpp"
#include "Screen/Layout.hpp"
#include "Look/FontDescription.hpp"
#include "Resources.hpp"
#include "Formatter/ByteSizeFormatter.hpp"

#include "ui/canvas/Canvas.hpp"

#include <algorithm>
#include <cstdint>

ProgressWindow::ProgressWindow(ContainerWindow &parent) noexcept
  :background_color(GlobalSettings::dark_mode
                    ? COLOR_DARK_THEME_BACKGROUND : COLOR_WHITE),
   dark_mode(GlobalSettings::dark_mode)
{
  message.clear();

  PixelRect rc = parent.GetClientRect();
  WindowStyle style;
  style.Hide();
  Create(parent, rc, style);

  // Load progress bar background
  bitmap_progress_border.Load(IDB_PROGRESSBORDER);

  // Determine text height
  font.Load(FontDescription(Layout::FontScale(10)));
  text_height = font.GetHeight();
  if (const unsigned spacing = font.GetLineSpacing();
      spacing > text_height)
    text_height = spacing;

  UpdateLayout(GetClientRect());

  // Initialize progress bar
  progress_bar.Create(*this, progress_bar_position);

  // Set progress bar step size and range
  SetRange(0, 1000);
  SetStep(50);

  // Show dialog
  ShowOnTop();
}

void
ProgressWindow::UpdateLayout(PixelRect rc) noexcept
{
  const unsigned height = rc.GetHeight();

  // Make progress bar height proportional to window height, and at
  // least as tall as one line so a label fits inside the bar.
  const unsigned progress_height = std::max(height / 20, text_height);
  const unsigned progress_horizontal_border = progress_height / 2;
  const unsigned progress_border_height = progress_height * 2;

  logo_position = rc;
  logo_position.bottom -= progress_border_height;

  message_position = rc;
  message_position.bottom -= progress_border_height + height / 48;
  message_position.top = message_position.bottom -
    text_height * message_lines;

  bottom_position = rc;
  bottom_position.top = bottom_position.bottom - progress_border_height;

  progress_bar_position.left = bottom_position.left + progress_horizontal_border;
  progress_bar_position.right = bottom_position.right - progress_horizontal_border;
  progress_bar_position.top = bottom_position.top + progress_horizontal_border;
  progress_bar_position.bottom = bottom_position.bottom - progress_horizontal_border;
}

static void
FormatSpeed(StaticString<16> &out, unsigned per_sec, bool bytes) noexcept
{
  /* A fast unnamed counter is a byte stream that did not call
     SetProgressBytes().  A slow one is lines, fixes, or a percent. */
  if (!bytes && per_sec < 1024) {
    out.Format("%u/s", per_sec);
    return;
  }

  char size[16];
  FormatByteSize(size, sizeof(size), per_sec);
  out.Format("%s/s", size);
}

static constexpr unsigned MAX_ETA_SECS = 99u * 3600u;

static void
FormatSeconds(StaticString<16> &eta, unsigned secs) noexcept
{
  const unsigned capped = secs > MAX_ETA_SECS ? MAX_ETA_SECS : secs;
  const unsigned hours = capped / 3600u;
  const unsigned minutes = (capped / 60u) % 60u;
  const unsigned seconds = capped % 60u;
  if (hours > 0)
    eta.Format("%u:%02u:%02u", hours, minutes, seconds);
  else
    eta.Format("%u:%02u", minutes, seconds);
}

/** First sample replaces the shown value.  Later ones move it
    three quarters of the way toward the new sample. */
static int
SmoothToward(int shown, int sample) noexcept
{
  if (shown < 0)
    return sample;
  return (shown * 3 + sample) / 4;
}

static StaticString<64>
FormatBarLabel(unsigned percent, unsigned done, unsigned total,
               int eta_secs, int per_sec, bool bytes) noexcept
{
  StaticString<16> eta("--:--");
  if (done >= total)
    eta = "0:00";
  else if (eta_secs >= 0)
    FormatSeconds(eta, unsigned(eta_secs));

  StaticString<16> speed("--");
  if (per_sec >= 0)
    FormatSpeed(speed, unsigned(per_sec), bytes);

  /* Digits, a percent sign, and the speed text.  Nothing here is a
     sentence, so it stays out of the translation catalog. */
  StaticString<64> text;
  text.Format("%u%% (%u/%u)  %s  %s",
              percent, done, total, speed.c_str(), eta.c_str());
  return text;
}

void
ProgressWindow::ResetRate() noexcept
{
  rate_started = false;
  rate_done = 0;
  speed_done = 0;
  shown_secs = -1;
  shown_rate = -1;
}

void
ProgressWindow::UpdateRate(unsigned done, unsigned total) noexcept
{
  /* The first tick is often a burst.  Time from the tick after
     that, and keep the placeholders until three seconds of that
     run exist. */
  const unsigned sample = have_bytes ? byte_count : done;
  const auto now = std::chrono::steady_clock::now();

  if (done == 0 || done < rate_done || sample < speed_done) {
    ResetRate();
    return;
  }

  if (!rate_started) {
    rate_started = true;
    rate_start = now;
    rate_done = done;
    speed_done = sample;
    return;
  }

  if (done <= rate_done && sample <= speed_done)
    return;

  const auto elapsed = now - rate_start;
  if (elapsed < std::chrono::seconds(3))
    return;

  const auto elapsed_ms =
    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
  const unsigned elapsed_count = unsigned(elapsed_ms.count());

  if (done > rate_done) {
    const unsigned got = done - rate_done;
    const unsigned remain = total - done;
    const uint64_t secs =
      uint64_t(elapsed_count) * remain / got / 1000u;
    const int estimate = secs > MAX_ETA_SECS
      ? int(MAX_ETA_SECS)
      : int(secs);
    shown_secs = SmoothToward(shown_secs, estimate);
  }

  if (sample > speed_done && elapsed_count > 0) {
    const unsigned got = sample - speed_done;
    const unsigned per_sec =
      unsigned(uint64_t(got) * 1000u / elapsed_count);
    shown_rate = SmoothToward(shown_rate, int(per_sec));
  }
}

void
ProgressWindow::UpdateBarLabel(unsigned value) noexcept
{
  if (range_max <= range_min) {
    progress_bar.SetText("");
    return;
  }

  if (value < range_min)
    value = range_min;
  if (value > range_max)
    value = range_max;

  const unsigned total = range_max - range_min;
  const unsigned done = value - range_min;
  UpdateRate(done, total);

  const unsigned percent =
    static_cast<unsigned>(uint64_t{done} * 100u / total);
  progress_bar.SetText(FormatBarLabel(percent, done, total,
                                       shown_secs, shown_rate,
                                       have_bytes));
}

static unsigned
CountMessageLines(const char *text) noexcept
{
  unsigned lines = 1;
  for (const char *p = text; *p != '\0'; ++p)
    if (*p == '\n')
      ++lines;

  /* Keep the two-line splash gap.  Cap so a long caption cannot
     cover the whole logo. */
  if (lines < 2)
    lines = 2;
  if (lines > 6)
    lines = 6;
  return lines;
}

void
ProgressWindow::SetMessage(const char *text) noexcept
{
  AssertThread();

  if (text == nullptr)
    text = "";

  message = text;

  const unsigned lines = CountMessageLines(message.c_str());
  if (lines != message_lines) {
    message_lines = lines;
    UpdateLayout(GetClientRect());
    Invalidate();
  } else
    Invalidate(message_position);
}

void
ProgressWindow::SetRange(unsigned min_value, unsigned max_value) noexcept
{
  range_min = min_value;
  range_max = max_value;
  ResetRate();
  have_bytes = false;
  byte_count = 0;
  progress_bar.SetRange(min_value, max_value);
  UpdateBarLabel(progress_bar.GetValue());
}

void
ProgressWindow::SetByteCount(unsigned bytes) noexcept
{
  AssertThread();

  /* The first byte report replaces a count baseline, so the speed
     is not a mix of lines and bytes. */
  if (!have_bytes || bytes < byte_count) {
    speed_done = bytes;
    shown_rate = -1;
  }

  have_bytes = true;
  byte_count = bytes;
  UpdateBarLabel(progress_bar.GetValue());
}

void
ProgressWindow::SetStep(unsigned size) noexcept
{
  progress_bar.SetStep(size);
}

void
ProgressWindow::SetValue(unsigned value) noexcept
{
  AssertThread();

  progress_bar.SetValue(value);
  UpdateBarLabel(value);
}

void
ProgressWindow::Step() noexcept
{
  progress_bar.Step();
  UpdateBarLabel(progress_bar.GetValue());
}

void
ProgressWindow::OnResize(PixelSize new_size) noexcept
{
  ContainerWindow::OnResize(new_size);

  UpdateLayout(GetClientRect());

  if (progress_bar.IsDefined())
    progress_bar.Move(progress_bar_position);

  Invalidate();
}

void
ProgressWindow::OnPaint(Canvas &canvas) noexcept
{
  canvas.Clear(background_color);

  logo.draw(canvas, logo_position, dark_mode);

  // Draw progress bar background
  canvas.Stretch(bottom_position.GetTopLeft(), bottom_position.GetSize(),
                 bitmap_progress_border);

  canvas.Select(font);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(dark_mode ? COLOR_WHITE : COLOR_BLACK);
  canvas.DrawFormattedText(message_position, message.c_str(),
                           DT_CENTER);

  ContainerWindow::OnPaint(canvas);
}
