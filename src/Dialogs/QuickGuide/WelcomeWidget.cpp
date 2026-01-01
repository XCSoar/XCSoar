// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WelcomeWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Look/FontDescription.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Resources.hpp"
#include "Language/Language.hpp"
#include "util/OpenLink.hpp"
#include "util/StaticString.hxx"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"

#include <winuser.h>

PixelSize WelcomeWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

PixelSize WelcomeWidget::GetMaximumSize() const noexcept {
  return { Layout::FastScale(300), Layout::FastScale(500) };
}

void
WelcomeWidget::Initialise(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
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

  const DialogLook &look = UIGlobals::GetDialogLook();

  const Font &fontDefault = look.text_font;
  const Font &fontTitle = look.bold_font;

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
  PixelRect t0_rc{x, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t0_height = canvas.DrawFormattedText(t0_rc, t0, DT_LEFT);
  y += int(t0_height) + margin;
  canvas.Select(fontDefault);

  StaticString<1024> t1;
  t1 = _("To get the most out of XCSoar and to learn about its many "
         "functions in detail, it is highly recommended to read the "
         "Quick Guide or the complete documentation.");
  t1 += _T(" ");
  t1 += _("The manuals explain step by step how to use XCSoar "
          "efficiently and cover both basic and advanced features.");
  PixelRect t1_rc{x, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t1_height = canvas.DrawFormattedText(t1_rc, t1.c_str(), DT_LEFT);
  y += int(t1_height) + margin;

  StaticString<512> t2;
  t2 = _("Documentation is available in several languages, including "
         "English, German, French, and Portuguese.");
  t2 += _T(" ");
  t2 += _("You can always access the latest versions online:");
  PixelRect t2_rc{x, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t2_height = canvas.DrawFormattedText(t2_rc, t2.c_str(), DT_LEFT);
  y += int(t2_height) + margin;

  canvas.SetTextColor(COLOR_BLUE);
  const TCHAR *t3 = _("https://xcsoar.org/discover/manual.html");
  PixelRect t3_rc{x, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t3_height =
    canvas.DrawFormattedText(t3_rc, t3, DT_LEFT | DT_UNDERLINE);
  xcsoar_link_rect = {x, y, int(canvas.GetWidth()) - margin,
                      y + int(t3_height)};
  canvas.SetTextColor(COLOR_BLACK);
  y += int(t3_height) + margin;

  StaticString<1024> t4;
  t4 = _("The XCSoar documentation is actively maintained, but some "
         "topics may still be missing or outdated.");
  t4 += _T(" ");
  t4 += _("If you find mistakes or have ideas for improvement, "
          "please contribute — XCSoar is open source and thrives on "
          "community input:");
  PixelRect t4_rc{x, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t4_height = canvas.DrawFormattedText(t4_rc, t4.c_str(), DT_LEFT);
  y += int(t4_height) + margin;

  canvas.SetTextColor(COLOR_BLUE);
  const TCHAR *t5 = _("https://github.com/XCSoar/XCSoar");
  PixelRect t5_rc{x, y, int(canvas.GetWidth()) - margin,
                  int(canvas.GetHeight())};
  unsigned t5_height =
    canvas.DrawFormattedText(t5_rc, t5, DT_LEFT | DT_UNDERLINE);
  github_link_rect = {x, y, int(canvas.GetWidth()) - margin,
                      y + int(t5_height)};
  canvas.SetTextColor(COLOR_BLACK);
  y += int(t5_height) + margin;
}

bool WelcomeWindow::OnMouseUp(PixelPoint p) noexcept {
  if (xcsoar_link_rect.Contains(p)) {
    return OpenLink("https://xcsoar.org/discover/manual.html");
  }
  if (github_link_rect.Contains(p)) {
    return OpenLink("https://github.com/XCSoar/XCSoar");
  }
  return false;
}
