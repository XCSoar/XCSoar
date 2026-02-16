// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UIActions.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Input/InputEvents.hpp"
#include "MainWindow.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "FLARM/Glue.hpp"
#include "Gauge/BigTrafficWidget.hpp"
#include "Gauge/BigThermalAssistantWidget.hpp"
#include "Look/Look.hpp"
#include "HorizonWidget.hpp"

static bool force_shutdown = false;

void
UIActions::SignalShutdown(bool force)
{
  force_shutdown = force;
  CommonInterface::main_window->Close();
}

bool
UIActions::CheckShutdown()
{
  if (force_shutdown)
    return true;

  return ShowMessageBox(_("Quit program?"), "XCSoar",
                     MB_YESNO | MB_ICONQUESTION) == IDYES;

}

void
UIActions::ShowTrafficRadar()
{
  if (InputEvents::IsFlavour("Traffic"))
    return;

  LoadFlarmDatabases();

  CommonInterface::main_window->SetWidget(new TrafficWidget());
  InputEvents::SetFlavour("Traffic");
}

void
UIActions::ShowThermalAssistant()
{
  if (InputEvents::IsFlavour("TA"))
    return;

  auto ta_widget =
    new BigThermalAssistantWidget(CommonInterface::GetLiveBlackboard(),
                                  UIGlobals::GetLook().thermal_assistant_dialog);
  CommonInterface::main_window->SetWidget(ta_widget);
  InputEvents::SetFlavour("TA");
}

void
UIActions::ShowHorizon()
{
  if (InputEvents::IsFlavour("Horizon"))
    return;

  auto widget = new HorizonWidget();
  CommonInterface::main_window->SetWidget(widget);
  InputEvents::SetFlavour("Horizon");
}
