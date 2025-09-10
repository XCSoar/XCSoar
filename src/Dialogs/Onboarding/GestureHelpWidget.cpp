// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GestureHelpWidget.hpp"
#include "Screen/Layout.hpp"
#include "Look/FontDescription.hpp"
#include "Language/Language.hpp"
#include "ui/canvas/Canvas.hpp"

#include <winuser.h>

PixelSize GestureHelpWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

PixelSize GestureHelpWidget::GetMaximumSize() const noexcept {
  return { Layout::FastScale(300), Layout::FastScale(540) };
}

void GestureHelpWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept {
  WindowStyle style;
  style.Hide();
  auto w = std::make_unique<GestureHelpWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

void
GestureHelpWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();

  PixelSize img_size = du_img.GetSize();
  
  int margin = Layout::FastScale(10);
  int x_img = rc.left + margin;
  int x_letter = x_img + img_size.width + margin;
  int x_text = x_img + img_size.width + Layout::FastScale(5) + 2 * margin;
  int y = rc.top + margin;

  Font fontDefault;
  fontDefault.Load(FontDescription(Layout::VptScale(12), false));

  Font fontTitle;
  fontTitle.Load(FontDescription(Layout::VptScale(12), true));

  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(12), true, false, true));

  Font fontSmall;
  fontSmall.Load(FontDescription(Layout::VptScale(10)));

  canvas.Select(fontDefault);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);
  
  const TCHAR *info_text = _("The following gestures can be drawn on the map view.");
  PixelRect info_text_rc{
    margin,
    margin,
    int(canvas.GetWidth()) - margin,
    int(canvas.GetHeight())
  };
  unsigned info_text_height = canvas.DrawFormattedText(info_text_rc, info_text, DT_LEFT);
  
  y += int(info_text_height) + margin;
  
  canvas.Select(fontTitle);
  const TCHAR *basic_title_text = _("Basic gestures");
  PixelSize basic_title_ps = canvas.CalcTextSize(basic_title_text);
  canvas.DrawText({x_img, y}, basic_title_text);

  y += int(basic_title_ps.height) + margin;
  
  canvas.Select(fontDefault);

  canvas.Copy({x_img, y}, img_size, du_img, {0, 0});
  const TCHAR *du_text = _("Show main menu, also via double tap");
  PixelSize du_ps = canvas.CalcTextSize(du_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(du_ps.height / 2)}, du_text);

  canvas.Select(fontMono);
  const TCHAR *du_letter = _("✓");
  PixelSize du_letter_ps = canvas.CalcTextSize(du_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(du_letter_ps.height / 2)}, du_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, up_img, {0, 0});
  const TCHAR *up_text = _("Zoom in");
  PixelSize up_ps = canvas.CalcTextSize(up_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(up_ps.height / 2)}, up_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, down_img, {0, 0});
  const TCHAR *down_text = _("Zoom out");
  PixelSize down_ps = canvas.CalcTextSize(down_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(down_ps.height / 2)}, down_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, left_img, {0, 0});
  const TCHAR *left_text = _("Show next page");
  PixelSize left_ps = canvas.CalcTextSize(left_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(left_ps.height / 2)}, left_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, right_img, {0, 0});
  const TCHAR *right_text = _("Show previous page");
  PixelSize right_ps = canvas.CalcTextSize(right_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(right_ps.height / 2)}, right_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, urdl_img, {0, 0});
  const TCHAR *urdl_text = _("Pan-mode, also via two-finger pinch gesture");
  PixelSize urdl_ps = canvas.CalcTextSize(urdl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(urdl_ps.height / 2)}, urdl_text);

  canvas.Select(fontMono);
  const TCHAR *urdl_letter = _("P");
  PixelSize urdl_letter_ps = canvas.CalcTextSize(urdl_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(urdl_letter_ps.height / 2)}, urdl_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + 2 * margin;



  canvas.Select(fontTitle);

  const TCHAR *advanced_title_text = _("Advanced gestures");
  PixelSize advanced_title_ps = canvas.CalcTextSize(advanced_title_text);
  canvas.DrawText({x_img, y}, advanced_title_text);

  y += int(advanced_title_ps.height) + margin;

  canvas.Select(fontDefault);

  canvas.Copy({x_img, y}, img_size, dr_img, {0, 0});
  const TCHAR *dr_text = _("Show Select Waypoint list");
  PixelSize dr_ps = canvas.CalcTextSize(dr_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(dr_ps.height / 2)}, dr_text);

  canvas.Select(fontMono);
  const TCHAR *dr_letter = _("L");
  PixelSize dr_letter_ps = canvas.CalcTextSize(dr_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(dr_letter_ps.height / 2)}, dr_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, rd_img, {0, 0});
  const TCHAR *rd_text = _("Open Task Manager");
  PixelSize rd_ps = canvas.CalcTextSize(rd_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(rd_ps.height / 2)}, rd_text);

  canvas.Select(fontMono);
  const TCHAR *rd_letter = _("T");
  PixelSize rd_letter_ps = canvas.CalcTextSize(rd_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(rd_letter_ps.height / 2)}, rd_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, dl_img, {0, 0});
  const TCHAR *dl_text = _("Show Alternates List");
  PixelSize dl_ps = canvas.CalcTextSize(dl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(dl_ps.height / 2)}, dl_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, ud_img, {0, 0});
  const TCHAR *ud_text = _("Enable Auto-Zoom");
  PixelSize ud_ps = canvas.CalcTextSize(ud_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(ud_ps.height / 2)}, ud_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, ldr_img, {0, 0});
  const TCHAR *ldr_text = _("Show checklist");
  PixelSize ldr_ps = canvas.CalcTextSize(ldr_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(ldr_ps.height / 2)}, ldr_text);

  canvas.Select(fontMono);
  const TCHAR *ldr_letter = _("C");
  PixelSize ldr_letter_ps = canvas.CalcTextSize(ldr_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(ldr_letter_ps.height / 2)}, ldr_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, urd_img, {0, 0});
  const TCHAR *urd_text = _("Show Analysis dialogue");
  PixelSize urd_ps = canvas.CalcTextSize(urd_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(urd_ps.height / 2)}, urd_text);

  canvas.Select(fontMono);
  const TCHAR *urd_letter = _("A");
  PixelSize urd_letter_ps = canvas.CalcTextSize(urd_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(urd_letter_ps.height / 2)}, urd_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, ldrdl_img, {0, 0});
  const TCHAR *ldrdl_text = _("Open Status dialogue");
  PixelSize ldrd_ps = canvas.CalcTextSize(ldrdl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(ldrd_ps.height / 2)}, ldrdl_text);

  canvas.Select(fontMono);
  const TCHAR *ldrdl_letter = _("S");
  PixelSize ldrdl_letter_ps = canvas.CalcTextSize(ldrdl_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(ldrdl_letter_ps.height / 2)}, ldrdl_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, uldr_img, {0, 0});
  const TCHAR *uldr_text = _("Access quick menu");
  PixelSize uldr_ps = canvas.CalcTextSize(uldr_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(uldr_ps.height / 2)}, uldr_text);

  canvas.Select(fontMono);
  const TCHAR *uldr_letter = _("Q");
  PixelSize uldr_letter_ps = canvas.CalcTextSize(uldr_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(uldr_letter_ps.height / 2)}, uldr_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, rl_img, {0, 0});
  const TCHAR *rl_text = _("FLARM: Switches selected aircraft displaying values such as climb rate relative altitude, etc.");
  PixelSize rl_ps = canvas.CalcTextSize(rl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(rl_ps.height / 2)}, rl_text);

  y += int(img_size.height) + margin;

  canvas.Select(fontSmall);
  const TCHAR *aresti_info_text = _("Gestures are displayed in Aresti notation, where the circle indicates the start and the dash indicates the end.");
  PixelRect aresti_rc{x_img, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  canvas.DrawFormattedText(aresti_rc, aresti_info_text, DT_LEFT);
}
