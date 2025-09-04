// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Widget/CreateWindowWidget.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "Resources.hpp"
#include "Look/FontDescription.hpp"

class GestureHelpWindow final : public PaintWindow {
protected:
  void OnPaint(Canvas &canvas) noexcept override;
};

void
GestureHelpWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.ClearWhite();
  
  Bitmap du_img(IDB_GESTURE_DU);
  Bitmap up_img(IDB_GESTURE_UP);
  Bitmap down_img(IDB_GESTURE_DOWN);
  Bitmap left_img(IDB_GESTURE_LEFT);
  Bitmap right_img(IDB_GESTURE_RIGHT);
  Bitmap urdl_img(IDB_GESTURE_URDL);
  Bitmap dr_img(IDB_GESTURE_DR);
  Bitmap rd_img(IDB_GESTURE_RD);
  Bitmap dl_img(IDB_GESTURE_DL);
  Bitmap ud_img(IDB_GESTURE_UD);
  Bitmap ldr_img(IDB_GESTURE_LDR);
  Bitmap urd_img(IDB_GESTURE_URD);
  Bitmap ldrdl_img(IDB_GESTURE_LDRDL);

  PixelSize img_size = du_img.GetSize();
  
  int margin = Layout::FastScale(10);
  int x_img = rc.left + Layout::FastScale(10);
  int x_letter = x_img + img_size.width + margin;
  int x_text = x_img + img_size.width + 30 + 2 * margin;
  int y = rc.top + Layout::FastScale(10);

  Font font;
  font.Load(FontDescription(Layout::VptScale(12), true));
  canvas.Select(font);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);



  const TCHAR *basic_title_text = _T("Basic gestures");
  PixelSize basic_title_ps = canvas.CalcTextSize(basic_title_text);
  canvas.DrawText({x_img, y}, basic_title_text);

  y += int(basic_title_ps.height) + margin;
  
  font.Load(FontDescription(Layout::VptScale(12), false));

  canvas.Copy({x_img, y}, img_size, du_img, {0, 0});
  const TCHAR *du_text = _T("Show main menu");
  PixelSize du_ps = canvas.CalcTextSize(du_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(du_ps.height / 2)}, du_text);

  font.Load(FontDescription(Layout::VptScale(12), true, false, true));
  const TCHAR *du_letter = _T("✓");
  PixelSize du_letter_ps = canvas.CalcTextSize(du_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(du_letter_ps.height / 2)}, du_letter);
  font.Load(FontDescription(Layout::VptScale(12), false));

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, up_img, {0, 0});
  const TCHAR *up_text = _T("Zoom in");
  PixelSize up_ps = canvas.CalcTextSize(up_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(up_ps.height / 2)}, up_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, down_img, {0, 0});
  const TCHAR *down_text = _T("Zoom out");
  PixelSize down_ps = canvas.CalcTextSize(down_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(down_ps.height / 2)}, down_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, left_img, {0, 0});
  const TCHAR *left_text = _T("Show next page");
  PixelSize left_ps = canvas.CalcTextSize(left_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(left_ps.height / 2)}, left_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, right_img, {0, 0});
  const TCHAR *right_text = _T("Show previous page");
  PixelSize right_ps = canvas.CalcTextSize(right_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(right_ps.height / 2)}, right_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, urdl_img, {0, 0});
  const TCHAR *urdl_text = _T("Pan-mode, also via two-finger pinch gesture.");
  PixelSize urdl_ps = canvas.CalcTextSize(urdl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(urdl_ps.height / 2)}, urdl_text);

  font.Load(FontDescription(Layout::VptScale(12), true, false, true));
  const TCHAR *urdl__letter = _T("P");
  PixelSize urdl_letter_ps = canvas.CalcTextSize(urdl__letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(urdl_letter_ps.height / 2)}, urdl__letter);
  font.Load(FontDescription(Layout::VptScale(12), false));

  y += int(img_size.height) + 2 * margin;



  font.Load(FontDescription(Layout::VptScale(12), true));

  const TCHAR *advanced_title_text = _T("Advanced gestures");
  PixelSize advanced_title_ps = canvas.CalcTextSize(advanced_title_text);
  canvas.DrawText({x_img, y}, advanced_title_text);

  y += int(advanced_title_ps.height) + margin;

  font.Load(FontDescription(Layout::VptScale(12), false));

  canvas.Copy({x_img, y}, img_size, dr_img, {0, 0});
  const TCHAR *dr_text = _T("Show Select Waypoint list");
  PixelSize dr_ps = canvas.CalcTextSize(dr_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(dr_ps.height / 2)}, dr_text);

  font.Load(FontDescription(Layout::VptScale(12), true, false, true));
  const TCHAR *dr_letter = _T("L");
  PixelSize dr_letter_ps = canvas.CalcTextSize(dr_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(dr_letter_ps.height / 2)}, dr_letter);
  font.Load(FontDescription(Layout::VptScale(12), false));

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, rd_img, {0, 0});
  const TCHAR *rd_text = _T("Open Task Manager");
  PixelSize rd_ps = canvas.CalcTextSize(rd_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(rd_ps.height / 2)}, rd_text);

  font.Load(FontDescription(Layout::VptScale(12), true, false, true));
  const TCHAR *rd_letter = _T("T");
  PixelSize rd_letter_ps = canvas.CalcTextSize(rd_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(rd_letter_ps.height / 2)}, rd_letter);
  font.Load(FontDescription(Layout::VptScale(12), false));

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, dl_img, {0, 0});
  const TCHAR *dl_text = _T("Show Alternates List");
  PixelSize d_letter_ps = canvas.CalcTextSize(dl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(d_letter_ps.height / 2)}, dl_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, ud_img, {0, 0});
  const TCHAR *ud_text = _T("Enable Auto-Zoom");
  PixelSize ud_ps = canvas.CalcTextSize(ud_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(ud_ps.height / 2)}, ud_text);

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, ldr_img, {0, 0});
  const TCHAR *ldr_text = _T("Show checklist");
  PixelSize ldr_ps = canvas.CalcTextSize(ldr_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(ldr_ps.height / 2)}, ldr_text);

  font.Load(FontDescription(Layout::VptScale(12), true, false, true));
  const TCHAR *ldr_letter = _T("C");
  PixelSize ldr_letter_ps = canvas.CalcTextSize(ldr_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(ldr_letter_ps.height / 2)}, ldr_letter);
  font.Load(FontDescription(Layout::VptScale(12), false));

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, urd_img, {0, 0});
  const TCHAR *urd_text = _T("Show Analysis dialogue");
  PixelSize urd_ps = canvas.CalcTextSize(urd_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(urd_ps.height / 2)}, urd_text);

  font.Load(FontDescription(Layout::VptScale(12), true, false, true));
  const TCHAR *urd_letter = _T("A");
  PixelSize urd_letter_ps = canvas.CalcTextSize(urd_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(urd_letter_ps.height / 2)}, urd_letter);
  font.Load(FontDescription(Layout::VptScale(12), false));

  y += int(img_size.height) + margin;

  canvas.Copy({x_img, y}, img_size, ldrdl_img, {0, 0});
  const TCHAR *ldrdl_text = _T("Open Status dialogue");
  PixelSize ldrd_letter_ps = canvas.CalcTextSize(ldrdl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(ldrd_letter_ps.height / 2)}, ldrdl_text);

  font.Load(FontDescription(Layout::VptScale(12), true, false, true));
  const TCHAR *ldrdl_letter = _T("S");
  PixelSize ldrdl_letter_ps = canvas.CalcTextSize(ldrdl_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(ldrdl_letter_ps.height / 2)}, ldrdl_letter);
  font.Load(FontDescription(Layout::VptScale(12), false));

  y += int(img_size.height) + margin;



  font.Load(FontDescription(Layout::VptScale(10)));
  const TCHAR *dot_info_text = _T("The dot is the end of the line.");
  canvas.DrawText({x_img, y}, dot_info_text);
}

static std::unique_ptr<Window>
CreateGestureHelpWidget(ContainerWindow &parent, const PixelRect &rc,
               WindowStyle style)
{
  auto window = std::make_unique<GestureHelpWindow>();
  window->Create(parent, rc, style);
  return window;
}

void
dlgOnboardingShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Onboarding"));

  auto widget = std::make_unique<CreateWindowWidget>(
    CreateGestureHelpWidget
  );

  dialog.FinishPreliminary(std::move(widget));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
