// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigurationWidget.hpp"
#include "QuickGuideLayoutContext.hpp"
#include "Screen/Layout.hpp"
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
#include "Airspace/Patterns.hpp"
#include "Waypoint/Patterns.hpp"

PixelSize ConfigurationWidget::GetMinimumSize() const noexcept {
  return { Layout::FastScale(200), Layout::FastScale(200) };
}

unsigned
ConfigurationWindow::Layout(Canvas *canvas, const PixelRect &rc,
                            ConfigurationWindow *window) noexcept
{
  QuickGuideLayoutContext ctx(canvas, rc, window);
  const int x_cb = ctx.GetCheckboxTextX();

  // Map
  {
    const auto path = Profile::GetPath(ProfileKeys::MapFile);
    ctx.DrawCheckbox(path != nullptr);
  }
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Download the map for your region"))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::SITE_FILES_1, x_cb,
               _("Config → System → Site Files"))) + ctx.margin;

  // Waypoints
  {
    const auto paths = Profile::GetMultiplePaths(ProfileKeys::WaypointFileList,
                                                 WAYPOINT_FILE_PATTERNS);
    ctx.DrawCheckbox(paths.size() > 0);
  }
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Download waypoints for your region"))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::SITE_FILES_2, x_cb,
               _("Config → System → Site Files"))) + ctx.margin;

  // Airspace
  {
    const auto paths = Profile::GetMultiplePaths(ProfileKeys::AirspaceFileList,
                                                 AIRSPACE_FILE_PATTERNS);
    ctx.DrawCheckbox(paths.size() > 0);
  }
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Download airspaces for your region"))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::SITE_FILES_3, x_cb,
               _("Config → System → Site Files"))) + ctx.margin;

  // Aircraft polar
  {
    const bool has_plane_path = Profile::GetPath("PlanePath") != nullptr;
    const bool has_polar = has_plane_path &&
      CommonInterface::GetComputerSettings().plane.polar_shape.IsValid();
    ctx.DrawCheckbox(has_polar);
  }
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Add your aircraft and, most importantly, select "
                 "the corresponding polar curve and activate the "
                 "aircraft, so that the flight computer can calculate "
                 "everything correctly."))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::PLANE_POLAR, x_cb,
               _("Config → Setup Plane → New → Polar → List")))
           + ctx.margin;

  // Pilot name
  {
    const char *name = Profile::Get(ProfileKeys::PilotName);
    ctx.DrawCheckbox(name != nullptr && !StringIsEmpty(name));
  }
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Set your name."))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::SETUP_LOGGER_1, x_cb,
               _("Config → System → Setup → Logger"))) + ctx.margin;

  // Pilot weight
  {
    const char *weight = Profile::Get(ProfileKeys::CrewWeightTemplate);
    ctx.DrawCheckbox(weight != nullptr && !StringIsEmpty(weight));
  }
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Set your default weight."))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::SETUP_LOGGER_2, x_cb,
               _("Config → System → Setup → Logger"))) + ctx.margin;

  // Timezone (UTC offset)
  {
    int utc_offset;
    ctx.DrawCheckbox(Profile::Get(ProfileKeys::UTCOffsetSigned, utc_offset));
  }
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Set your timezone."))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::SETUP_TIME, x_cb,
               _("Config → System → Setup → Time"))) + ctx.margin;

  // Home waypoint
  {
    int home_waypoint;
    ctx.DrawCheckbox(Profile::Get(ProfileKeys::HomeWaypoint, home_waypoint));
  }
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Set home airfield."))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawTextBlock(ctx.GetMonoFont(), x_cb,
               _("Tap waypoint on map → select waypoint → Details "
                 "→ next page → Set as New Home"))) + ctx.margin;

  // InfoBoxes (no checkbox)
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Configure the pages and InfoBoxes as you prefer.")))
           + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::LOOK_INFO_BOX_SETS, x_cb,
               _("Config → System → Look → InfoBox Sets")))
           + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::LOOK_PAGES, x_cb,
               _("Config → System → Look → Pages"))) + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::YOUTUBE_TUTORIAL, x_cb,
               _T("https://youtube.com/user/M24Tom/playlists"))) + ctx.margin;

  // NMEA devices (no checkbox)
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), x_cb,
               _("Configure NMEA devices via Bluetooth or USB-Serial.")))
           + ctx.margin / 2;
  ctx.y += int(ctx.DrawLinkLine(LinkAction::DEVICES, x_cb,
               _("Config → Devices"))) + ctx.margin;

  ctx.y += ctx.margin;

  // Replay (full-width text)
  StaticString<1024> replay_text;
  replay_text = _("The easiest way to get familiar with XCSoar is to load an "
                  "existing flight and use the replay feature to explore its "
                  "functions.");
  replay_text += _T(" ");
  replay_text += _("To do this, copy an IGC file into the XCSoarData/logs "
                   "folder.");
  replay_text += _T(" ");
  replay_text += _("Then you can select this file under Config → Config → "
                   "Replay and start the flight simulation.");
  ctx.y += int(ctx.DrawTextBlock(ctx.GetTextFont(), ctx.x,
                                 replay_text.c_str())) + ctx.margin;

  ctx.y += int(ctx.DrawTextBlock(ctx.GetSmallFont(), ctx.x,
               _("Maps, waypoints, etc. downloaded using the "
                 "download feature can be updated in the file "
                 "manager (Config → Config → File manager)."))) + ctx.margin;

  ctx.y += int(ctx.DrawTextBlock(ctx.GetSmallFont(), ctx.x,
               _("Files are stored in the operating system of the "
                 "mobile device and can also be replaced and "
                 "supplemented there, e.g., to add custom waypoints "
                 "for a competition. On iOS, these files are located "
                 "here: Files app → On my iPhone → XCSoar → "
                 "XCSoarData. On Android, these files are located "
                 "here: Android → media → org.xcsoar.")));

  return ctx.GetHeight();
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

bool
ConfigurationWindow::OnLinkActivated(std::size_t index) noexcept
{
  return HandleLink(static_cast<LinkAction>(index));
}
