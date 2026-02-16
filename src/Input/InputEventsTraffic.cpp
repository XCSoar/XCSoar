// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "PageActions.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Gauge/BigTrafficWidget.hpp"
#include "FLARM/Glue.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"

void
InputEvents::eventFLARMRadar([[maybe_unused]] const char *misc)
{
  if (StringIsEqual(misc, _T("ForceToggle"))) {
    CommonInterface::main_window->ToggleForceFLARMRadar();
  } else
    CommonInterface::main_window->ToggleSuppressFLARMRadar();
}

// FLARM Traffic
// Displays the FLARM traffic dialog
void
InputEvents::eventFlarmTraffic([[maybe_unused]] const char *misc)
{
  PageActions::ShowTrafficRadar();
}

void
InputEvents::eventTraffic(const char *misc)
{
  LoadFlarmDatabases();

  if (StringIsEqual(misc, _T("show"))) {
    PageActions::ShowTrafficRadar();
    return;
  }

  TrafficWidget *traffic_widget = (TrafficWidget *)
    CommonInterface::main_window->GetFlavourWidget(_T("Traffic"));
  if (traffic_widget == nullptr)
    return;

  if (StringIsEqual(misc, _T("zoom auto toggle"))) {
    traffic_widget->ToggleAutoZoom();
  } else if (StringIsEqual(misc, _T("zoom in"))) {
    traffic_widget->ZoomIn();
  } else if (StringIsEqual(misc, _T("zoom out"))) {
    traffic_widget->ZoomOut();
  } else if (StringIsEqual(misc, _T("northup toggle"))) {
    traffic_widget->ToggleNorthUp();
  } else if (StringIsEqual(misc, _T("details"))) {
    traffic_widget->OpenDetails();
  } else if (StringIsEqual(misc, _T("label toggle"))) {
    traffic_widget->SwitchData();
  }
}

void
InputEvents::eventFlarmDetails([[maybe_unused]] const char *misc)
{
  LoadFlarmDatabases();
  TrafficListDialog();
}
