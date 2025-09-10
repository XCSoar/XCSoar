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
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/VScrollWidget.hpp"
#include "Profile/Profile.hpp"
#include "Form/CheckBox.hpp"
#include "system/Path.hpp"
#include "util/StringCompare.hxx"

#include <winuser.h>

class GestureHelpWindow final : public PaintWindow {
  Bitmap down_img{IDB_GESTURE_DOWN}, dl_img{IDB_GESTURE_DL}, dr_img{IDB_GESTURE_DR},
         du_img{IDB_GESTURE_DU}, left_img{IDB_GESTURE_LEFT}, ldr_img{IDB_GESTURE_LDR},
         ldrdl_img{IDB_GESTURE_LDRDL}, right_img{IDB_GESTURE_RIGHT}, rd_img{IDB_GESTURE_RD},
         rl_img{IDB_GESTURE_RL}, up_img{IDB_GESTURE_UP}, ud_img{IDB_GESTURE_UD},
         uldr_img{IDB_GESTURE_ULDR}, urd_img{IDB_GESTURE_URD}, urdl_img{IDB_GESTURE_URDL};
protected:
  void OnPaint(Canvas &canvas) noexcept override;
};

class GestureHelpWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override {
    return { Layout::FastScale(200), Layout::FastScale(200) };
  }
  
  PixelSize GetMaximumSize() const noexcept override {
    return { Layout::FastScale(300), Layout::FastScale(540) };
  }

  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();
    auto w = std::make_unique<GestureHelpWindow>();
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
};

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

class DontShowAgainWidget : public NullWidget {
  CheckBoxControl checkbox;
  LargeTextWidget info_text;
  const DialogLook &look;
  const TCHAR *fixed_text = _("This popup can be accessed anytime from the menu under Info → Getting Started.");

public:
  explicit DontShowAgainWidget(const DialogLook &_look) : info_text(_look), look(_look) {}

  PixelRect MakeCheckboxRect(const PixelRect &rc) const noexcept {
    const unsigned cb_height = ::Layout::GetMinimumControlHeight();

    PixelRect rect;
    rect.left   = rc.left;
    rect.top    = rc.top;
    rect.right  = rc.right;
    rect.bottom = rect.top + cb_height;
    return rect;
  }

  PixelRect MakeTextRect(const PixelRect &rc, const PixelRect &cb_rect) const noexcept {
    PixelRect rect = rc;
    const unsigned spacing = 5;
    rect.top = cb_rect.bottom + spacing;
    return rect;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
  WindowStyle style;
  style.Hide();
  style.TabStop();
  
  checkbox.Create(parent, look, _("Don't show onboarding dialog on startup"),
    MakeCheckboxRect(rc), style,
    [](bool value) {
      Profile::Set(ProfileKeys::HideOnboardingDialogOnStartup, value);
	  Profile::Save();
    }
  );
  
  info_text.Prepare(parent, MakeTextRect(rc, MakeCheckboxRect(rc)));
  info_text.SetText(fixed_text);
  }

  void Show(const PixelRect &rc) noexcept override {
    bool hide_onboarding_dialog_on_startup = false;
	Profile::Get(ProfileKeys::HideOnboardingDialogOnStartup, hide_onboarding_dialog_on_startup);
    checkbox.SetState(hide_onboarding_dialog_on_startup);
    auto cb_rect = MakeCheckboxRect(rc);
    checkbox.MoveAndShow(cb_rect);
  
    info_text.Show(MakeTextRect(rc, cb_rect));
  }
  
  void Hide() noexcept override {
    checkbox.FastHide();
    info_text.Hide();
  }
  
  void Move(const PixelRect &rc) noexcept override {
    auto cb_rect = MakeCheckboxRect(rc);
    checkbox.Move(cb_rect);
    info_text.Move(MakeTextRect(rc, cb_rect));
  }
};

class ConfigurationWindow final : public PaintWindow {
protected:
  void OnPaint(Canvas &canvas) noexcept override;
};

class ConfigurationWidget final : public WindowWidget {
public:
  PixelSize GetMinimumSize() const noexcept override {
    return { Layout::FastScale(200), Layout::FastScale(200) };
  }
  
  PixelSize GetMaximumSize() const noexcept override {
    return { Layout::FastScale(300), Layout::FastScale(1000) };
  }

  void Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    WindowStyle style;
    style.Hide();
    auto w = std::make_unique<ConfigurationWindow>();
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
};

void
ConfigurationWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();
  
  int margin = Layout::FastScale(10);
  int x = rc.left + margin;
  int x_text = x + Layout::FastScale(20);
  int y = rc.top + margin;
  int icon_offset = Layout::FastScale(1);
  constexpr const char* undone = "[ ]";
  constexpr const char* done   = "[X]";

  Font fontDefault;
  fontDefault.Load(FontDescription(Layout::VptScale(12), false));

  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(10), false, false, true));

  Font fontSmall;
  fontSmall.Load(FontDescription(Layout::VptScale(10), false));

  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_BLACK);

  // Map
  canvas.Select(fontMono);
  const auto c1 = Profile::GetPath(ProfileKeys::MapFile);
  canvas.DrawText({x, y + icon_offset}, c1 == nullptr ? undone : done);
  canvas.Select(fontDefault);
  const TCHAR *t1 = _("Download the map for your region");
  PixelRect t1_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t1_height = canvas.DrawFormattedText(t1_rc, t1, DT_LEFT);
  y += int(t1_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l1 = _("Config → System → Site Files");
  PixelRect l1_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l1_height = canvas.DrawFormattedText(l1_rc, l1, DT_LEFT);
  y += int(l1_height) + margin;
  
  // Waypoints
  canvas.Select(fontMono);
  const auto c2 = Profile::GetPath(ProfileKeys::WaypointFile);
  canvas.DrawText({x, y + icon_offset}, c2 == nullptr ? undone : done);
  canvas.Select(fontDefault);
  const TCHAR *t2 = _("Download waypoints for your region");
  PixelRect t2_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t2_height = canvas.DrawFormattedText(t2_rc, t2, DT_LEFT);
  y += int(t2_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l2 = _("Config → System → Site Files");
  PixelRect l2_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l2_height = canvas.DrawFormattedText(l2_rc, l2, DT_LEFT);
  y += int(l2_height) + margin;

  // Airspace
  canvas.Select(fontMono);
  const auto c3 = Profile::GetMultiplePaths(ProfileKeys::AirspaceFileList);
  canvas.DrawText({x, y + icon_offset}, c3.size() == 0 ? undone : done);
  canvas.Select(fontDefault);
  const TCHAR *t3 = _("Download airspaces for your region");
  PixelRect t3_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t3_height = canvas.DrawFormattedText(t3_rc, t3, DT_LEFT);
  y += int(t3_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l3 = _("Config → System → Site Files");
  PixelRect l3_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l3_height = canvas.DrawFormattedText(l3_rc, l3, DT_LEFT);
  y += int(l3_height) + margin;

  // Aircraft polar
  canvas.Select(fontMono);
  const char *c4 = Profile::Get(ProfileKeys::Polar); // TODO does not work
  canvas.DrawText({x, y + icon_offset}, (c4 == nullptr || StringIsEmpty(c4)) ? undone : done);
  canvas.Select(fontDefault);
  const TCHAR *t4 = _("Add your aircraft and, most importantly, select the corresponding polar curve and activate the aircraft, so that the flight computer can calculate everything correctly.");
  PixelRect t4_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t4_height = canvas.DrawFormattedText(t4_rc, t4, DT_LEFT);
  y += int(t4_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l4 = _("Config → Setup Plane → New → Polar → List");
  PixelRect l4_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l4_height = canvas.DrawFormattedText(l4_rc, l4, DT_LEFT);
  y += int(l4_height) + margin;

  // Pilot name
  canvas.Select(fontMono);
  const char *c5 = Profile::Get(ProfileKeys::PilotName);
  canvas.DrawText({x, y + icon_offset}, (c5 == nullptr || StringIsEmpty(c5)) ? undone : done);
  canvas.Select(fontDefault);
  const TCHAR *t5 = _("Set your name.");
  PixelRect t5_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t5_height = canvas.DrawFormattedText(t5_rc, t5, DT_LEFT);
  y += int(t5_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l5 = _("Config → System → Setup → Logger");
  PixelRect l5_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l5_height = canvas.DrawFormattedText(l5_rc, l5, DT_LEFT);
  y += int(l5_height) + margin;

  // Pilot weight
  canvas.Select(fontMono);
  const char *c6 = Profile::Get(ProfileKeys::CrewWeightTemplate);
	canvas.DrawText({x, y + icon_offset}, (c6 == nullptr || StringIsEmpty(c6) || (intptr_t)c6 <= 0) ? undone : done);
  canvas.Select(fontDefault);
  const TCHAR *t6 = _("Set your default weight.");
  PixelRect t6_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t6_height = canvas.DrawFormattedText(t6_rc, t6, DT_LEFT);
  y += int(t6_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l6 = _("Config → System → Setup → Logger");
  PixelRect l6_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l6_height = canvas.DrawFormattedText(l6_rc, l6, DT_LEFT);
  y += int(l6_height) + margin;

  // Timezone (UTC offset)
  canvas.Select(fontMono);
  int utc_offset;
  canvas.DrawText({x, y + icon_offset}, (!Profile::Get(ProfileKeys::UTCOffsetSigned, utc_offset) ? undone : done));
  canvas.Select(fontDefault);
  const TCHAR *t7 = _("Set your timezone.");
  PixelRect t7_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t7_height = canvas.DrawFormattedText(t7_rc, t7, DT_LEFT);
  y += int(t7_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l7 = _("Config → System → Setup → Time");
  PixelRect l7_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l7_height = canvas.DrawFormattedText(l7_rc, l7, DT_LEFT);
  y += int(l7_height) + margin;

  // Home waypoint
  canvas.Select(fontMono);
  int home_waypoint;
  canvas.DrawText({x, y + icon_offset}, (!Profile::Get(ProfileKeys::HomeWaypoint, home_waypoint) ? undone : done)); // TODO HomeWaypoint vs HomeLocation
  canvas.Select(fontDefault);
  const TCHAR *t8 = _("Set home airfield.");
  PixelRect t8_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t8_height = canvas.DrawFormattedText(t8_rc, t8, DT_LEFT);
  y += int(t8_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l8 = _("Tap waypoint on map → select waypoint → Details → next page → Set as New Home");
  PixelRect l8_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l8_height = canvas.DrawFormattedText(l8_rc, l8, DT_LEFT);
  y += int(l8_height) + margin;

  // InfoBoxes
  canvas.Select(fontDefault);
  const TCHAR *t9 = _("Configure the pages and InfoBoxes as you prefer.");
  PixelRect t9_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t9_height = canvas.DrawFormattedText(t9_rc, t9, DT_LEFT);
  y += int(t9_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l9 = _("Config → System → Look → InfoBox Sets");
  PixelRect l9_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l9_height = canvas.DrawFormattedText(l9_rc, l9, DT_LEFT);
  y += int(l9_height) + margin / 2;
  const TCHAR *l9b = _("Config → System → Look → Pages");
  PixelRect l9b_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l9b_height = canvas.DrawFormattedText(l9b_rc, l9b, DT_LEFT);
  y += int(l9b_height) + margin / 2;
  const TCHAR *l9c = _("https://youtube.com/user/M24Tom/playlists"); // https://youtube.com/playlist?list=PLb36zVafnfctzNG7yJoNXc2x8HnX5J9j7
  PixelRect l9c_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l9c_height = canvas.DrawFormattedText(l9c_rc, l9c, DT_LEFT);
  y += int(l9c_height) + margin;
  
  // NMEA devices
  canvas.Select(fontDefault);
  const TCHAR *t10 = _("Configure NMEA devices via Bluetooth or USB-Serial.");
  PixelRect t10_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t10_height = canvas.DrawFormattedText(t10_rc, t10, DT_LEFT);
  y += int(t10_height) + margin / 2;
  canvas.Select(fontMono);
  const TCHAR *l10 = _("Config → Devices");
  PixelRect l10_rc{x_text, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned l10_height = canvas.DrawFormattedText(l10_rc, l10, DT_LEFT);
  y += int(l10_height) + margin;

  y += margin;

  canvas.Select(fontSmall);
  const TCHAR *t98 = _("Maps, waypoints, etc. downloaded using the download feature can be updated in the file manager (Config. → Config. → File manager).");
  PixelRect t98_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t98_height = canvas.DrawFormattedText(t98_rc, t98, DT_LEFT);
  y += int(t98_height) + margin;

  canvas.Select(fontSmall);
  const TCHAR *t99 = _("Files are stored in the operating system of the mobile device and can also be replaced and supplemented there, e.g., to add custom waypoints for a competition. On iOS, these files are located here: Files app → On my iPhone → XCSoar → XCSoarData. On Android, these files are located here: Android → media → org.xcsoar.");
  PixelRect t99_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  canvas.DrawFormattedText(t99_rc, t99, DT_LEFT);
  y += margin;
}


void
dlgOnboardingShowModal()
{
  std::vector titles = {
    _("Getting started: Gestures"),
    _("Getting started: Configuration"),
    _("Getting started: Show again"),
  };

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Getting started"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button, dialog.MakeModalResultCallback(mrOK));

  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<GestureHelpWidget>(), look));
  pager->Add(std::make_unique<VScrollWidget>(std::make_unique<ConfigurationWidget>(), look));
  pager->Add(std::make_unique<DontShowAgainWidget>(look));

  pager->SetPageFlippedCallback([&dialog, &titles, &pager=*pager](){
    dialog.SetCaption(titles[pager.GetCurrentIndex()]);
  });

  dialog.SetCaption(titles[pager->GetCurrentIndex()]);

  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();
}
