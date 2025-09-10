// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigurationWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Font.hpp"
#include "Look/FontDescription.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "system/Path.hpp"
#include "util/StringCompare.hxx"

#include <winuser.h>

PixelSize ConfigurationWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

PixelSize ConfigurationWidget::GetMaximumSize() const noexcept {
  return { Layout::FastScale(300), Layout::FastScale(1000) };
}

void ConfigurationWidget::Initialise(ContainerWindow &parent, const PixelRect &rc) noexcept {
  WindowStyle style;
  style.Hide();
  auto w = std::make_unique<ConfigurationWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

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
  const TCHAR *t98 = _("Maps, waypoints, etc. downloaded using the download feature can be updated in the file manager (Config → Config → File manager).");
  PixelRect t98_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  unsigned t98_height = canvas.DrawFormattedText(t98_rc, t98, DT_LEFT);
  y += int(t98_height) + margin;

  canvas.Select(fontSmall);
  const TCHAR *t99 = _("Files are stored in the operating system of the mobile device and can also be replaced and supplemented there, e.g., to add custom waypoints for a competition. On iOS, these files are located here: Files app → On my iPhone → XCSoar → XCSoarData. On Android, these files are located here: Android → media → org.xcsoar.");
  PixelRect t99_rc{x, y, int(canvas.GetWidth()) - margin, int(canvas.GetHeight())};
  canvas.DrawFormattedText(t99_rc, t99, DT_LEFT);
}
