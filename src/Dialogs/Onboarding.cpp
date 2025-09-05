// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Onboarding.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/VScrollWidget.hpp"
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
  Bitmap du_img{IDB_GESTURE_DU}, up_img{IDB_GESTURE_UP}, down_img{IDB_GESTURE_DOWN},
         left_img{IDB_GESTURE_LEFT}, right_img{IDB_GESTURE_RIGHT}, urdl_img{IDB_GESTURE_URDL},
         dr_img{IDB_GESTURE_DR}, rd_img{IDB_GESTURE_RD}, dl_img{IDB_GESTURE_DL},
         ud_img{IDB_GESTURE_UD}, ldr_img{IDB_GESTURE_LDR}, urd_img{IDB_GESTURE_URD},
         ldrdl_img{IDB_GESTURE_LDRDL};
protected:
  void OnPaint(Canvas &canvas) noexcept override;
};

class GestureHelpWidget final : public WindowWidget {
  std::unique_ptr<GestureHelpWindow> window;

public:
  PixelSize GetMinimumSize() const noexcept override {
    return { Layout::FastScale(200), Layout::FastScale(200) };
  }
  
  PixelSize GetMaximumSize() const noexcept override {
    return { Layout::FastScale(300), Layout::FastScale(450) };
  }

  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    WindowStyle style;
    window = std::make_unique<GestureHelpWindow>();
    window->Create(parent, rc, style);
	SetWindow(std::move(window));
  }

  void Show(const PixelRect &rc) noexcept override {
    (void)rc;
    if (window) {
      window->Show();
    }
  }
};

void
GestureHelpWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();

  PixelSize img_size = du_img.GetSize();
  
  int margin = Layout::FastScale(10);
  int x_img = rc.left + Layout::FastScale(10);
  int x_letter = x_img + img_size.width + margin;
  int x_text = x_img + img_size.width + Layout::FastScale(5) + 2 * margin;
  int y = rc.top + Layout::FastScale(10);

  Font fontDefault;
  fontDefault.Load(FontDescription(Layout::VptScale(12), false));

  Font fontTitle;
  fontTitle.Load(FontDescription(Layout::VptScale(12), true));

  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(12), true, false, true));

  Font fontSmall;
  fontSmall.Load(FontDescription(Layout::VptScale(10)));

  canvas.Select(fontTitle);
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);



  const TCHAR *basic_title_text = _("Basic gestures");
  PixelSize basic_title_ps = canvas.CalcTextSize(basic_title_text);
  canvas.DrawText({x_img, y}, basic_title_text);

  y += int(basic_title_ps.height) + margin;
  
  canvas.Select(fontDefault);

  canvas.Copy({x_img, y}, img_size, du_img, {0, 0});
  const TCHAR *du_text = _("Show main menu");
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
  const TCHAR *urdl_text = _("Pan-mode, also via two-finger pinch gesture.");
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
  PixelSize dl_letter_ps = canvas.CalcTextSize(dl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(dl_letter_ps.height / 2)}, dl_text);

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
  PixelSize ldrd_letter_ps = canvas.CalcTextSize(ldrdl_text);
  canvas.DrawText({x_text, y + int(img_size.height / 2) - int(ldrd_letter_ps.height / 2)}, ldrdl_text);

  canvas.Select(fontMono);
  const TCHAR *ldrdl_letter = _("S");
  PixelSize ldrdl_letter_ps = canvas.CalcTextSize(ldrdl_letter);
  canvas.DrawText({x_letter, y + int(img_size.height / 2) - int(ldrdl_letter_ps.height / 2)}, ldrdl_letter);
  canvas.Select(fontDefault);

  y += int(img_size.height) + margin;


  canvas.Select(fontSmall);
  const TCHAR *dot_info_text = _("The dot is the end of the line.");
  canvas.DrawText({x_img, y}, dot_info_text);
}

void
dlgOnboardingShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Onboarding"));

  auto inner = std::make_unique<GestureHelpWidget>();
  auto scroll = std::make_unique<VScrollWidget>(std::move(inner), look);

  dialog.FinishPreliminary(std::move(scroll));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
