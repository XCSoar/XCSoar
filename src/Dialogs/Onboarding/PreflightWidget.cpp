// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PreflightWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/FontDescription.hpp"
#include "Language/Language.hpp"
#include "util/ConvertString.hpp"

#include <winuser.h>
#include <fmt/format.h>

PixelSize PreflightWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

PixelSize PreflightWidget::GetMaximumSize() const noexcept {
  return { Layout::FastScale(300), Layout::FastScale(500) };
}

void PreflightWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept {
  WindowStyle style;
  style.Hide();
  auto w = std::make_unique<PreflightWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
PreflightWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();
  
  int margin = Layout::FastScale(10);
  int x = rc.left + margin;
  int x_indent = x + Layout::FastScale(17);
  int y = rc.top + margin;

  Font fontDefault;
  fontDefault.Load(FontDescription(Layout::VptScale(12), false));
  
  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(10), false, false, true));

  canvas.Select(fontDefault);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);

  const TCHAR *t0 = _("There are several things that should be set and checked before each flight.");
  PixelRect t0_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t0_height = canvas.DrawFormattedText(t0_rc, t0, DT_LEFT);
  y += int(t0_height) + margin;

  // 1. Checklist
  canvas.DrawText({x, y}, "1.)");
  std::string s1 = fmt::format("{} {}",
    _("It is possible to store a checklist."),
    _("To do this, an xcsoar-checklist.txt file must be added to the XCSoarData folder."));
  UTF8ToWideConverter t1(s1.c_str());
  PixelRect t1_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t1_height = canvas.DrawFormattedText(t1_rc, (const TCHAR *)t1, DT_LEFT);
  y += int(t1_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l1 = _("Info → Checklist");
  PixelRect l1_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l1_height = canvas.DrawFormattedText(l1_rc, l1, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l1_height) + margin;


  // 2. Aircraft / Polar
  canvas.DrawText({x, y}, "2.)");
  std::string s2 = fmt::format("{}",
    _("Select and activate the correct aircraft and polar configuration, so that weight and performance are accurate."));
  UTF8ToWideConverter t2(s2.c_str());
  PixelRect t2_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t2_height = canvas.DrawFormattedText(t2_rc, (const TCHAR *)t2, DT_LEFT);
  y += int(t2_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l2 = _("Config → Plane");
  PixelRect l2_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l2_height = canvas.DrawFormattedText(l2_rc, l2, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l2_height) + margin;

  // 3. Flight
  canvas.DrawText({x, y}, "3.)");
  std::string s3 = fmt::format("{}",
    _("Set flight parameters such as wing loading, bugs, QNH and maximum temperature."));
  UTF8ToWideConverter t3(s3.c_str());
  PixelRect t3_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t3_height = canvas.DrawFormattedText(t3_rc, (const TCHAR *)t3, DT_LEFT);
  y += int(t3_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l3 = _("Info → Flight");
  PixelRect l3_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l3_height = canvas.DrawFormattedText(l3_rc, l3, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l3_height) + margin;

  // 4. Wind
  canvas.DrawText({x, y}, "4.)");
  std::string s4 = fmt::format("{}",
    _("Configure wind data manually or enable auto wind to set speed and direction."));
  UTF8ToWideConverter t4(s4.c_str());
  PixelRect t4_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t4_height = canvas.DrawFormattedText(t4_rc, (const TCHAR *)t4, DT_LEFT);
  y += int(t4_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l4 = _("Info → Wind");
  PixelRect l4_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l4_height = canvas.DrawFormattedText(l4_rc, l4, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l4_height) + margin;

  // 5. Task
  canvas.DrawText({x, y}, "5.)");
  std::string s5 = fmt::format("{}",
    _("Create a task so XCSoar can guide navigation and provide return support."));
  UTF8ToWideConverter t5(s5.c_str());
  PixelRect t5_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t5_height = canvas.DrawFormattedText(t5_rc, (const TCHAR *)t5, DT_LEFT);
  y += int(t5_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l5 = _("Nav → Task Manager");
  PixelRect l5_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l5_height = canvas.DrawFormattedText(l5_rc, l5, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l5_height) + margin;
}
