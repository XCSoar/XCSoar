// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InternalLink.hpp"
#include "util/StringCompare.hxx"
#include "Dialogs/Dialogs.h"
#include "Dialogs/Device/DeviceListDialog.hpp"
#include "Dialogs/Plane/PlaneDialogs.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/dlgAnalysis.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/dlgGestureHelp.hpp"
#include "Dialogs/ReplayDialog.hpp"
#include "Dialogs/Settings/Panels/SiteConfigPanel.hpp"
#include "Dialogs/Settings/Panels/LoggerConfigPanel.hpp"
#include "Dialogs/Settings/Panels/TimeConfigPanel.hpp"
#include "Dialogs/Settings/Panels/InfoBoxesConfigPanel.hpp"
#include "Dialogs/Settings/Panels/PagesConfigPanel.hpp"
#include "Dialogs/Settings/Panels/WeGlideConfigPanel.hpp"
#include "Dialogs/Settings/Panels/WeatherConfigPanel.hpp"
#include "Dialogs/Settings/Panels/SafetyFactorsConfigPanel.hpp"
#include "Dialogs/Settings/Panels/TrackingConfigPanel.hpp"
#include "Dialogs/Settings/Panels/TerrainDisplayConfigPanel.hpp"
#include "Widget/Widget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Profile/Profile.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "DataComponents.hpp"
#include "Language/Language.hpp"

void
ShowConfigPanel(const char *title,
                std::unique_ptr<Widget> (*create_panel)())
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, title);
  auto panel = create_panel();
  dialog.FinishPreliminary(std::move(panel));
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  if (dialog.GetChanged())
    Profile::Save();
}

/**
 * Table of config panel links that all follow the
 * ShowConfigPanel(title, factory) pattern.
 */
struct ConfigPanelLink {
  const char *path;
  const char *title;
  std::unique_ptr<Widget> (*factory)();
};

static constexpr ConfigPanelLink config_panel_links[] = {
  {"config/site-files", N_("Site Files"), CreateSiteConfigPanel},
  {"config/logger",     N_("Logger"),     CreateLoggerConfigPanel},
  {"config/time",       N_("Time"),       CreateTimeConfigPanel},
  {"config/infoboxes",  N_("InfoBox Sets"), CreateInfoBoxesConfigPanel},
  {"config/pages",      N_("Pages"),      CreatePagesConfigPanel},
  {"config/weglide",    N_("WeGlide"),    CreateWeGlideConfigPanel},
  {"config/weather",    N_("Weather"),    CreateWeatherConfigPanel},
  {"config/safety",     N_("Safety Factors"), CreateSafetyFactorsConfigPanel},
  {"config/tracking",   N_("Tracking"),   CreateTrackingConfigPanel},
  {"config/terrain",    N_("Terrain Display"), CreateTerrainDisplayConfigPanel},
};

/**
 * Table of simple dialog links that call a void() handler.
 */
struct SimpleDialogLink {
  const char *path;
  void (*handler)();
};

static constexpr SimpleDialogLink simple_dialog_links[] = {
  {"dialog/checklist", dlgChecklistShowModal},
  {"dialog/flight",    dlgBasicSettingsShowModal},
  {"dialog/wind",      ShowWindSettingsDialog},
  {"dialog/task",      dlgTaskManagerShowModal},
  {"dialog/gestures",  dlgGestureHelpShowModal},
};

bool
HandleInternalLink(const char *url)
{
  // Must start with xcsoar://
  if (!StringStartsWith(url, "xcsoar://"))
    return false;

  const char *path = url + 9;  // Skip "xcsoar://"

  /* ---- Config panel table ---- */

  for (const auto &entry : config_panel_links) {
    if (StringIsEqual(path, entry.path)) {
      ShowConfigPanel(gettext(entry.title), entry.factory);
      return true;
    }
  }

  /* ---- Simple dialog table ---- */

  for (const auto &entry : simple_dialog_links) {
    if (StringIsEqual(path, entry.path)) {
      entry.handler();
      return true;
    }
  }

  /* ---- Special-case config dialogs ---- */

  if (StringIsEqual(path, "config/devices")) {
    if (backend_components != nullptr &&
        backend_components->device_blackboard != nullptr) {
      ShowDeviceList(*backend_components->device_blackboard,
                     backend_components->devices.get());
    }
    return true;
  }

  if (StringIsEqual(path, "config/planes")) {
    dlgPlanesShowModal();
    return true;
  }

  /* ---- Special-case action dialogs ---- */

  if (StringIsEqual(path, "dialog/analysis")) {
    if (backend_components == nullptr ||
        backend_components->glide_computer == nullptr ||
        data_components == nullptr)
      return true;  // Recognized but not executable without backend
    dlgAnalysisShowModal(*CommonInterface::main_window,
                         CommonInterface::main_window->GetLook(),
                         CommonInterface::Full(),
                         *backend_components->glide_computer,
                         data_components->airspaces.get(),
                         data_components->terrain.get());
    return true;
  }

  if (StringIsEqual(path, "dialog/status")) {
    dlgStatusShowModal(-1);
    return true;
  }

  if (StringIsEqual(path, "dialog/credits")) {
    dlgCreditsShowModal(*CommonInterface::main_window);
    return true;
  }

  if (StringIsEqual(path, "dialog/replay")) {
    if (backend_components != nullptr &&
        backend_components->replay &&
        !CommonInterface::MovementDetected())
      ShowReplayDialog(*backend_components->replay);
    return true;
  }

  return false;
}
