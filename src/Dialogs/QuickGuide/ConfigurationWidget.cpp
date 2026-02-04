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
#include "util/OpenLink.hpp"
#include "util/StaticString.hxx"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Device/DeviceListDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Settings/Panels/SiteConfigPanel.hpp"
#include "Dialogs/Settings/Panels/LoggerConfigPanel.hpp"
#include "Dialogs/Settings/Panels/TimeConfigPanel.hpp"
#include "Dialogs/Settings/Panels/InfoBoxesConfigPanel.hpp"
#include "Dialogs/Settings/Panels/PagesConfigPanel.hpp"
#include "Dialogs/Plane/PlaneDialogs.hpp"
#include "UtilsSettings.hpp"
#include "BackendComponents.hpp"
#include "Components.hpp"
#include "Interface.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Airspace/Patterns.hpp"
#include "Waypoint/Patterns.hpp"

#include <winuser.h>

PixelSize ConfigurationWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

unsigned
ConfigurationWindow::Layout(Canvas *canvas, const PixelRect &rc,
                            ConfigurationWindow *window) noexcept
{
  const int margin = Layout::FastScale(10);
  const int x = rc.left + margin;
  const int x_text = x + Layout::FastScale(20);
  int y = rc.top + margin;
  const int icon_offset = Layout::FastScale(1);

  const int right = rc.right - margin;
  const DialogLook &look = UIGlobals::GetDialogLook();
  const Font &fontDefault = look.text_font;
  const Font &fontSmall = look.small_font;

  Font fontMono;
  fontMono.Load(FontDescription(Layout::VptScale(10), false, false, true));

  TextRenderer renderer;

  if (canvas != nullptr) {
    canvas->SetBackgroundTransparent();
    canvas->SetTextColor(COLOR_BLACK);
  }

  auto DrawTextBlock = [&](const Font &font, int left, const TCHAR *text,
                           unsigned format=DT_LEFT) {
    if (canvas != nullptr) {
      canvas->Select(font);
      PixelRect text_rc{left, y, right, rc.bottom};
      return canvas->DrawFormattedText(text_rc, text, format);
    }

    const int width = right > left ? right - left : 0;
    return renderer.GetHeight(font, width, text);
  };

  auto DrawLinkLine = [&](LinkAction link, const TCHAR *text) {
    if (canvas != nullptr && window != nullptr) {
      canvas->Select(fontMono);
      PixelRect link_rc{x_text, y, right, rc.bottom};
      return window->DrawLink(*canvas, link, link_rc, text);
    }

    const int width = right > x_text ? right - x_text : 0;
    return renderer.GetHeight(fontMono, width, text);
  };

  if (canvas != nullptr)
    canvas->Select(fontMono);

  // Map
  if (canvas != nullptr) {
    const auto c1 = Profile::GetPath(ProfileKeys::MapFile);
    canvas->DrawText({x, y + icon_offset}, c1 == nullptr ? _T("[ ]") : _T("[X]"));
  }
  const TCHAR *t1 = _("Download the map for your region");
  y += int(DrawTextBlock(fontDefault, x_text, t1)) + margin / 2;
  const TCHAR *l1 = _("Config → System → Site Files");
  y += int(DrawLinkLine(LinkAction::SITE_FILES_1, l1)) + margin;

  // Waypoints
  if (canvas != nullptr) {
    const auto c2 =
      Profile::GetMultiplePaths(ProfileKeys::WaypointFileList,
                                WAYPOINT_FILE_PATTERNS);
    canvas->Select(fontMono);
    canvas->DrawText({x, y + icon_offset}, c2.size() == 0 ? _T("[ ]") : _T("[X]"));
  }
  const TCHAR *t2 = _("Download waypoints for your region");
  y += int(DrawTextBlock(fontDefault, x_text, t2)) + margin / 2;
  const TCHAR *l2 = _("Config → System → Site Files");
  y += int(DrawLinkLine(LinkAction::SITE_FILES_2, l2)) + margin;

  // Airspace
  if (canvas != nullptr) {
    const auto c3 =
      Profile::GetMultiplePaths(ProfileKeys::AirspaceFileList,
                                AIRSPACE_FILE_PATTERNS);
    canvas->Select(fontMono);
    canvas->DrawText({x, y + icon_offset}, c3.size() == 0 ? _T("[ ]") : _T("[X]"));
  }
  const TCHAR *t3 = _("Download airspaces for your region");
  y += int(DrawTextBlock(fontDefault, x_text, t3)) + margin / 2;
  const TCHAR *l3 = _("Config → System → Site Files");
  y += int(DrawLinkLine(LinkAction::SITE_FILES_3, l3)) + margin;

  // Aircraft polar
  if (canvas != nullptr) {
    const bool has_plane_path = Profile::GetPath("PlanePath") != nullptr;
    const bool has_polar = has_plane_path &&
      CommonInterface::GetComputerSettings().plane.polar_shape.IsValid();
    canvas->Select(fontMono);
    canvas->DrawText({x, y + icon_offset}, !has_polar ? _T("[ ]") : _T("[X]"));
  }
  const TCHAR *t4 = _("Add your aircraft and, most importantly, select "
                      "the corresponding polar curve and activate the "
                      "aircraft, so that the flight computer can calculate "
                      "everything correctly.");
  y += int(DrawTextBlock(fontDefault, x_text, t4)) + margin / 2;
  const TCHAR *l4 = _("Config → Setup Plane → New → Polar → List");
  y += int(DrawLinkLine(LinkAction::PLANE_POLAR, l4)) + margin;

  // Pilot name
  if (canvas != nullptr) {
    const char *c5 = Profile::Get(ProfileKeys::PilotName);
    canvas->Select(fontMono);
    canvas->DrawText({x, y + icon_offset},
                     (c5 == nullptr || StringIsEmpty(c5)) ? _T("[ ]") : _T("[X]"));
  }
  const TCHAR *t5 = _("Set your name.");
  y += int(DrawTextBlock(fontDefault, x_text, t5)) + margin / 2;
  const TCHAR *l5 = _("Config → System → Setup → Logger");
  y += int(DrawLinkLine(LinkAction::SETUP_LOGGER_1, l5)) + margin;

  // Pilot weight
  if (canvas != nullptr) {
    const char *c6 = Profile::Get(ProfileKeys::CrewWeightTemplate);
    canvas->Select(fontMono);
    canvas->DrawText({x, y + icon_offset},
                     (c6 == nullptr || StringIsEmpty(c6)) ? _T("[ ]") : _T("[X]"));
  }
  const TCHAR *t6 = _("Set your default weight.");
  y += int(DrawTextBlock(fontDefault, x_text, t6)) + margin / 2;
  const TCHAR *l6 = _("Config → System → Setup → Logger");
  y += int(DrawLinkLine(LinkAction::SETUP_LOGGER_2, l6)) + margin;

  // Timezone (UTC offset)
  if (canvas != nullptr) {
    int utc_offset;
    canvas->Select(fontMono);
    canvas->DrawText({x, y + icon_offset},
                    (!Profile::Get(ProfileKeys::UTCOffsetSigned, utc_offset)
                     ? _T("[ ]") : _T("[X]")));
  }
  const TCHAR *t7 = _("Set your timezone.");
  y += int(DrawTextBlock(fontDefault, x_text, t7)) + margin / 2;
  const TCHAR *l7 = _("Config → System → Setup → Time");
  y += int(DrawLinkLine(LinkAction::SETUP_TIME, l7)) + margin;

  // Home waypoint
  if (canvas != nullptr) {
    int home_waypoint;
    canvas->Select(fontMono);
    canvas->DrawText({x, y + icon_offset},
                    (!Profile::Get(ProfileKeys::HomeWaypoint, home_waypoint)
                     ? _T("[ ]") : _T("[X]")));
  }
  const TCHAR *t8 = _("Set home airfield.");
  y += int(DrawTextBlock(fontDefault, x_text, t8)) + margin / 2;
  const TCHAR *l8 = _("Tap waypoint on map → select waypoint → Details "
                      "→ next page → Set as New Home");
  if (canvas != nullptr) {
    canvas->Select(fontMono);
    PixelRect l8_rc{x_text, y, right, rc.bottom};
    y += int(canvas->DrawFormattedText(l8_rc, l8, DT_LEFT)) + margin;
  } else {
    const int width = right > x_text ? right - x_text : 0;
    y += int(renderer.GetHeight(fontMono, width, l8)) + margin;
  }

  // InfoBoxes
  const TCHAR *t9 = _("Configure the pages and InfoBoxes as you prefer.");
  y += int(DrawTextBlock(fontDefault, x_text, t9)) + margin / 2;
  const TCHAR *l9 = _("Config → System → Look → InfoBox Sets");
  y += int(DrawLinkLine(LinkAction::LOOK_INFO_BOX_SETS, l9)) + margin / 2;
  const TCHAR *l9b = _("Config → System → Look → Pages");
  y += int(DrawLinkLine(LinkAction::LOOK_PAGES, l9b)) + margin / 2;
  const TCHAR *l9c = _T("https://youtube.com/user/M24Tom/playlists");
  y += int(DrawLinkLine(LinkAction::YOUTUBE_TUTORIAL, l9c)) + margin;

  // NMEA devices
  const TCHAR *t10 = _("Configure NMEA devices via Bluetooth or USB-Serial.");
  y += int(DrawTextBlock(fontDefault, x_text, t10)) + margin / 2;
  const TCHAR *l10 = _("Config → Devices");
  y += int(DrawLinkLine(LinkAction::DEVICES, l10)) + margin;

  y += margin;

  // Replay
  StaticString<1024> t97;
  t97 = _("The easiest way to get familiar with XCSoar is to load an "
          "existing flight and use the replay feature to explore its "
          "functions.");
  t97 += _T(" ");
  t97 += _("To do this, copy an IGC file into the XCSoarData/logs "
          "folder.");
  t97 += _T(" ");
  t97 += _("Then you can select this file under Config → Config → "
          "Replay and start the flight simulation.");
  y += int(DrawTextBlock(fontDefault, x, t97.c_str())) + margin;

  const TCHAR *t98 = _("Maps, waypoints, etc. downloaded using the "
                       "download feature can be updated in the file "
                       "manager (Config → Config → File manager).");
  y += int(DrawTextBlock(fontSmall, x, t98)) + margin;

  const TCHAR *t99 = _("Files are stored in the operating system of the "
                       "mobile device and can also be replaced and "
                       "supplemented there, e.g., to add custom waypoints "
                       "for a competition. On iOS, these files are located "
                       "here: Files app → On my iPhone → XCSoar → "
                       "XCSoarData. On Android, these files are located "
                       "here: Android → media → org.xcsoar.");
  y += int(DrawTextBlock(fontSmall, x, t99));

  return static_cast<unsigned>(y);
}

PixelSize ConfigurationWidget::GetMaximumSize() const noexcept {
  PixelSize size = GetMinimumSize();
  size.width = Layout::FastScale(300);

  unsigned width = size.width;
  if (IsDefined())
    width = GetWindow().GetSize().width;

  const PixelRect measure_rc{PixelPoint{0, 0}, PixelSize{width, 0u}};
  const unsigned height = ConfigurationWindow::Layout(nullptr, measure_rc, nullptr);
  if (height > size.height)
    size.height = height;

  return size;
}

void
ConfigurationWidget::Initialise(ContainerWindow &parent,
                                const PixelRect &rc) noexcept
{
  WindowStyle style;
  style.Hide();
  style.TabStop();
  auto w = std::make_unique<ConfigurationWindow>();
  w->Create(parent, rc, style);
  SetWindow(std::move(w));
}

bool
ConfigurationWidget::SetFocus() noexcept
{
  GetWindow().SetFocus();
  return true;
}

void
ConfigurationWindow::OnPaint(Canvas &canvas) noexcept
{
  const PixelRect rc = GetClientRect();

  canvas.Clear();
  Layout(&canvas, rc, this);
}

ConfigurationWindow::ConfigurationWindow() noexcept
  : QuickGuideLinkWindow()
{
  const auto count = static_cast<std::size_t>(LinkAction::COUNT);
  link_rects.resize(count);
}

bool
ConfigurationWindow::HandleLink(LinkAction link) noexcept
{
  switch (link) {
  case LinkAction::SITE_FILES_1:
  case LinkAction::SITE_FILES_2:
  case LinkAction::SITE_FILES_3:{
    const DialogLook &look = UIGlobals::GetDialogLook();
    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        look, _("Site Files"));
    auto panel = CreateSiteConfigPanel();
    dialog.FinishPreliminary(std::move(panel));
    dialog.AddButton(_("Close"), mrOK);
    dialog.ShowModal();
    if (dialog.GetChanged())
      Profile::Save();
    return true;
  }

  case LinkAction::PLANE_POLAR:
    dlgPlanesShowModal();
    return true;

  case LinkAction::SETUP_LOGGER_1:
  case LinkAction::SETUP_LOGGER_2: {
    const DialogLook &look = UIGlobals::GetDialogLook();
    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        look, _("Logger"));
    auto panel = CreateLoggerConfigPanel();
    dialog.FinishPreliminary(std::move(panel));
    dialog.AddButton(_("Close"), mrOK);
    dialog.ShowModal();
    if (dialog.GetChanged())
      Profile::Save();
    return true;
  }

  case LinkAction::SETUP_TIME: {
    const DialogLook &look = UIGlobals::GetDialogLook();
    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        look, _("Time"));
    auto panel = CreateTimeConfigPanel();
    dialog.FinishPreliminary(std::move(panel));
    dialog.AddButton(_("Close"), mrOK);
    dialog.ShowModal();
    if (dialog.GetChanged())
      Profile::Save();
    return true;
  }

  case LinkAction::LOOK_INFO_BOX_SETS: {
    const DialogLook &look = UIGlobals::GetDialogLook();
    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        look, _("InfoBox Sets"));
    auto panel = CreateInfoBoxesConfigPanel();
    dialog.FinishPreliminary(std::move(panel));
    dialog.AddButton(_("Close"), mrOK);
    dialog.ShowModal();
    if (dialog.GetChanged())
      Profile::Save();
    return true;
  }

  case LinkAction::LOOK_PAGES: {
    const DialogLook &look = UIGlobals::GetDialogLook();
    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        look, _("Pages"));
    auto panel = CreatePagesConfigPanel();
    dialog.FinishPreliminary(std::move(panel));
    dialog.AddButton(_("Close"), mrOK);
    dialog.ShowModal();
    if (dialog.GetChanged())
      Profile::Save();
    return true;
  }

  case LinkAction::YOUTUBE_TUTORIAL:
    return OpenLink("https://youtube.com/playlist?list="
                    "PLb36zVafnfctzNG7yJoNXc2x8HnX5J9j7");

  case LinkAction::DEVICES:
    if (backend_components != nullptr &&
        backend_components->device_blackboard != nullptr) {
      ShowDeviceList(*backend_components->device_blackboard,
                     backend_components->devices.get());
      return true;
    }
    return false;

  case LinkAction::COUNT:
    break;
  }

  return false;
}

unsigned
ConfigurationWindow::DrawLink(Canvas &canvas, LinkAction link, PixelRect rc,
                              const TCHAR *text) noexcept
{
  return QuickGuideLinkWindow::DrawLink(canvas,
                                        static_cast<std::size_t>(link),
                                        rc, text);
}

bool
ConfigurationWindow::OnLinkActivated(std::size_t index) noexcept
{
  return HandleLink(static_cast<LinkAction>(index));
}
