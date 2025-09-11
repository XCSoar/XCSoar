// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WelcomeWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/FontDescription.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Resources.hpp"
#include "Language/Language.hpp"
#include "util/ConvertString.hpp"

#include <winuser.h>
#include <fmt/format.h>

PixelSize WelcomeWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

PixelSize WelcomeWidget::GetMaximumSize() const noexcept {
  return { Layout::FastScale(300), Layout::FastScale(500) };
}

void
WelcomeWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  auto w = std::make_unique<WelcomeWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
WelcomeWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();

  int margin = Layout::FastScale(10);
  int x = rc.left + margin;
  int y = rc.top + margin;

  Font fontDefault;
  fontDefault.Load(FontDescription(Layout::VptScale(12), false));

  Font fontTitle;
  fontTitle.Load(FontDescription(Layout::VptScale(12), true));

  canvas.Select(fontDefault);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);

  Bitmap logo(IDB_LOGO_HD);
  PixelSize logo_size = logo.GetSize();
  int x_logo = rc.GetWidth() / 2 - (logo_size.width / 2);
  canvas.Copy({x_logo, y}, logo_size, logo, {0, 0});
  y += int(logo_size.height) + margin / 2;

  Bitmap title(IDB_TITLE_HD);
  PixelSize title_size = title.GetSize();
  int x_title = rc.GetWidth() / 2 - (title_size.width / 2);
  canvas.Copy({x_title, y}, title_size, title, {0, 0});
  y += int(title_size.height) + margin * 2;


  canvas.Select(fontTitle);
  const TCHAR *t0 = _("Welcome to XCSoar");
  PixelRect t0_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t0_height = canvas.DrawFormattedText(t0_rc, t0, DT_LEFT);
  y += int(t0_height) + margin;
  canvas.Select(fontDefault);

  std::string s1 = fmt::format("{} {}",
    _("To get the most out of XCSoar and to learn about its many functions in detail, "
      "it is highly recommended to read the Quick Guide or the complete documentation."),
    _("The manuals explain step by step how to use XCSoar efficiently "
      "and cover both basic and advanced features."));
  UTF8ToWideConverter t1(s1.c_str());
  PixelRect t1_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t1_height = canvas.DrawFormattedText(t1_rc, (const TCHAR *)t1, DT_LEFT);
  y += int(t1_height) + margin;

  std::string s2 = fmt::format("{} {}",
    _("Documentation is available in several languages, including English, German, French, and Portuguese."),
    _("You can always access the latest versions online:"));
  UTF8ToWideConverter t2(s2.c_str());
  PixelRect t2_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t2_height = canvas.DrawFormattedText(t2_rc, (const TCHAR *)t2, DT_LEFT);
  y += int(t2_height) + margin;

  const TCHAR *t3 = _("https://xcsoar.org/discover/manual.html");
  PixelRect t3_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t3_height = canvas.DrawFormattedText(t3_rc, t3, DT_LEFT);
  y += int(t3_height) + margin;

  std::string s4 = fmt::format("{} {}",
    _("The XCSoar documentation is actively maintained, but some topics may still be missing or outdated."),
    _("If you find mistakes or have ideas for improvement, please contribute — XCSoar is open source and thrives on community input:"));
  UTF8ToWideConverter t4(s4.c_str());
  PixelRect t4_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t4_height = canvas.DrawFormattedText(t4_rc, (const TCHAR *)t4, DT_LEFT);
  y += int(t4_height) + margin;

  const TCHAR *t5 = _("https://github.com/XCSoar/XCSoar");
  PixelRect t5_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t5_height = canvas.DrawFormattedText(t5_rc, t5, DT_LEFT);
  y += int(t5_height) + margin;
}
