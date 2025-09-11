// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PostflightWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/FontDescription.hpp"
#include "Language/Language.hpp"
#include "util/ConvertString.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"

#include <winuser.h>
#include <fmt/format.h>

PixelSize PostflightWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

PixelSize PostflightWidget::GetMaximumSize() const noexcept {
  return { Layout::FastScale(300), Layout::FastScale(500) };
}

void PostflightWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept {
  WindowStyle style;
  style.Hide();
  auto w = std::make_unique<PostflightWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
PostflightWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();
  
  int margin = Layout::FastScale(10);
  int x = rc.left + margin;
  int x_indent = x + Layout::FastScale(17);
  int y = rc.top + margin;

  const DialogLook &look = UIGlobals::GetDialogLook();

  const Font &fontDefault = look.text_font;
  
  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(10), false, false, true));

  canvas.Select(fontDefault);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);

  const TCHAR *t0 = _("After your flight, there are several steps to check and complete.");
  PixelRect t0_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t0_height = canvas.DrawFormattedText(t0_rc, t0, DT_LEFT);
  y += int(t0_height) + margin;

  // 1. Download Flight Log
  canvas.DrawText({x, y}, "1.)");
  std::string s1 = fmt::format("{} {}",
    _("Download flight logs from your connected NMEA device, such as a FLARM unit or another supported logger."),
    _("List available logs and save them in XCSoarData/logs, from where they can be manually uploaded to other devices or platforms such as WeGlide."));
  UTF8ToWideConverter t1(s1.c_str());
  PixelRect t1_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t1_height = canvas.DrawFormattedText(t1_rc, static_cast<const TCHAR *>(t1), DT_LEFT);
  y += int(t1_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l1 = _("Config → Devices → Flight download");
  PixelRect l1_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l1_height = canvas.DrawFormattedText(l1_rc, l1, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l1_height) + margin;

  // 2. Flight Analysis
  canvas.DrawText({x, y}, "2.)");
  std::string s2 = fmt::format("{}",
    _("Review statistical data from your flight such as your flight score, barograph, and glide polar analysis."));
  UTF8ToWideConverter t2(s2.c_str());
  PixelRect t2_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t2_height = canvas.DrawFormattedText(t2_rc, static_cast<const TCHAR *>(t2), DT_LEFT);
  y += int(t2_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l2 = _("Info → Analysis");
  PixelRect l2_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l2_height = canvas.DrawFormattedText(l2_rc, l2, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l2_height) + margin;

  // 3. Flight Status
  canvas.DrawText({x, y}, "3.)");
  std::string s3 = fmt::format("{}",
    _("Check detailed statistics and timing information of your flight."));
  UTF8ToWideConverter t3(s3.c_str());
  PixelRect t3_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t3_height = canvas.DrawFormattedText(t3_rc, static_cast<const TCHAR *>(t3), DT_LEFT);
  y += int(t3_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l3 = _("Info → Info → Status");
  PixelRect l3_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l3_height = canvas.DrawFormattedText(l3_rc, l3, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l3_height) + margin;

  // 4. Upload Flight
  canvas.DrawText({x, y}, "4.)");
  std::string s4 = fmt::format("{} {} {}",
    _("Flights can be uploaded directly from XCSoar to WeGlide."),
    _("For this, configure your WeGlide User ID and date of birth in the system setup."),
    _("You can find your User ID on weglide.org under 'My profile' by copying the numbers from the URL."));
  UTF8ToWideConverter t4(s4.c_str());
  PixelRect t4_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t4_height = canvas.DrawFormattedText(t4_rc, static_cast<const TCHAR *>(t4), DT_LEFT);
  y += int(t4_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l4 = _("Config → System → Setup → WeGlide");
  PixelRect l4_rc{x_indent, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l4_height = canvas.DrawFormattedText(l4_rc, l4, DT_LEFT);
  canvas.Select(fontDefault);
  y += int(l4_height) + margin;
}
