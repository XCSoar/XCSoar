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
#include "Hardware/SystemPower.hpp"

static bool force_shutdown = false;
static UIActions::ExitAction exit_action = UIActions::ExitAction::NONE;

void
UIActions::SignalShutdown(bool force)
{
  force_shutdown = force;
  exit_action = force ? ExitAction::QUIT : ExitAction::NONE;
  CommonInterface::main_window->Close();
}

#if defined(__linux__) && !defined(ANDROID)
static UIActions::ExitAction
ShowExitDialog(SystemPower::Capabilities capabilities) noexcept
{
  enum Result {
    QUIT = 100,
    REBOOT,
    POWER_OFF,
  };

  MessageBoxButton buttons[4];
  unsigned n_buttons = 0;
  buttons[n_buttons++] = {_("Quit"), QUIT};
  if (capabilities.reboot)
    buttons[n_buttons++] = {_("Restart"), REBOOT};
  if (capabilities.power_off)
    buttons[n_buttons++] = {_("Power off"), POWER_OFF};
  buttons[n_buttons++] = {_("Cancel"), IDCANCEL};

  const int result =
    ShowMessageBox(_("What do you want to do?"), "XCSoar",
                   std::span{buttons}.first(n_buttons), IDCANCEL);
  switch (result) {
  case QUIT:
    return UIActions::ExitAction::QUIT;

  case REBOOT:
    return UIActions::ExitAction::REBOOT;

  case POWER_OFF:
    return UIActions::ExitAction::POWER_OFF;
  }

  return UIActions::ExitAction::NONE;
}
#endif

bool
UIActions::CheckShutdown() noexcept
{
  if (force_shutdown)
    return true;

#if defined(__linux__) && !defined(ANDROID)
  const auto capabilities = SystemPower::GetCapabilities();
  if (capabilities.Any()) {
    exit_action = ShowExitDialog(capabilities);
    return exit_action != ExitAction::NONE;
  }
#endif

  if (ShowMessageBox(_("Quit program?"), "XCSoar",
                     MB_YESNO | MB_ICONQUESTION) != IDYES)
    return false;

  exit_action = ExitAction::QUIT;
  return true;
}

UIActions::ExitAction
UIActions::GetExitAction() noexcept
{
  return exit_action;
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
