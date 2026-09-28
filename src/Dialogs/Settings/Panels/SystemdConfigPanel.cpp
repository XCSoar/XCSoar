// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemdConfigPanel.hpp"

#include "Dialogs/Error.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Job/Job.hpp"
#include "Language/Language.hpp"
#include "Linux/SystemdServiceList.hpp"
#include "UIGlobals.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/ScopeMatch.hxx"
#include "lib/dbus/Systemd.hxx"
#include "ui/event/PeriodicTimer.hpp"
#include "util/ScopeExit.hxx"

#include <chrono>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

static constexpr auto refresh_interval         = std::chrono::seconds{1};
static constexpr int systemd_action_timeout_ms = 30000;

enum class SystemdAction {
  START,
  STOP,
  RESTART,
};

class SystemdActionJob final : public Job
{
  const SystemdAction action;
  const std::string unit_name;

public:
  SystemdActionJob(SystemdAction _action, const std::string &_unit_name)
      : action(_action), unit_name(_unit_name)
  {
  }

  void Run([[maybe_unused]] OperationEnvironment &env) override
  {
    auto connection = ODBus::Connection::GetSystemPrivate();
    AtScopeExit(&connection)
    {
      connection.Close();
    };

    const ODBus::ScopeMatch match{connection, Systemd::job_removed_match};

    switch (action) {
    case SystemdAction::START:
      Systemd::StartUnit(
        connection, unit_name.c_str(), "replace", systemd_action_timeout_ms);
      Systemd::EnableUnitFile(connection, unit_name.c_str());
      break;

    case SystemdAction::STOP:
      Systemd::StopUnit(
        connection, unit_name.c_str(), "replace", systemd_action_timeout_ms);
      Systemd::DisableUnitFile(connection, unit_name.c_str());
      break;

    case SystemdAction::RESTART:
      Systemd::RestartUnit(
        connection, unit_name.c_str(), "replace", systemd_action_timeout_ms);
      break;
    }
  }
};

struct ServiceStatus
{
  Systemd::ActiveState state = Systemd::ActiveState::INACTIVE;
  bool valid                 = false;
};

struct Fields
{
  std::vector<SystemdService> services;
  std::vector<ServiceStatus> statuses;
  GroupedListWidget *list = nullptr;
};

[[gnu::pure]]
static bool
CanToggle(const ServiceStatus &status) noexcept
{
  if (!status.valid) return false;

  switch (status.state) {
  case Systemd::ActiveState::ACTIVE:
  case Systemd::ActiveState::INACTIVE:
  case Systemd::ActiveState::FAILED:
    return true;

  case Systemd::ActiveState::ACTIVATING:
  case Systemd::ActiveState::DEACTIVATING:
  case Systemd::ActiveState::RELOADING:
    return false;
  }

  return false;
}

static void
ApplyState(const ServiceStatus &status,
           GroupedListWidget::ValueState &state) noexcept
{
  state.disabled = !CanToggle(status);

  if (!status.valid) {
    state.badge       = _("Unavailable");
    state.badge_style = GroupedListWidget::BadgeStyle::DANGER;
    return;
  }

  switch (status.state) {
  case Systemd::ActiveState::ACTIVE:
    state.badge       = _("On");
    state.badge_style = GroupedListWidget::BadgeStyle::SUCCESS;
    break;

  case Systemd::ActiveState::INACTIVE:
    state.text = _("Off");
    break;

  case Systemd::ActiveState::ACTIVATING:
    state.badge       = _("Starting...");
    state.badge_style = GroupedListWidget::BadgeStyle::WARNING;
    break;

  case Systemd::ActiveState::DEACTIVATING:
    state.badge       = _("Stopping...");
    state.badge_style = GroupedListWidget::BadgeStyle::WARNING;
    break;

  case Systemd::ActiveState::RELOADING:
    state.badge       = _("Reloading...");
    state.badge_style = GroupedListWidget::BadgeStyle::WARNING;
    break;

  case Systemd::ActiveState::FAILED:
    state.badge       = _("Failed");
    state.badge_style = GroupedListWidget::BadgeStyle::DANGER;
    break;
  }
}

static void
Refresh(Fields &fields) noexcept
{
  try {
    auto connection = ODBus::Connection::GetSystem();
    for (std::size_t i = 0; i < fields.services.size(); ++i) {
      try {
        fields.statuses[i].state = Systemd::GetUnitActiveState(
          connection, fields.services[i].unit_name.c_str());
        fields.statuses[i].valid = true;
      } catch (...) {
        fields.statuses[i].valid = false;
      }
    }
  } catch (...) {
    for (auto &status : fields.statuses)
      status.valid = false;
  }

  if (fields.list != nullptr) fields.list->UpdateValues();
}

static void
RunAction(Fields &fields,
          UI::PeriodicTimer &timer,
          SystemdAction action,
          const std::string &unit) noexcept
{
  timer.Cancel();
  try {
    SystemdActionJob job{action, unit};
    if (!JobDialog(UIGlobals::GetMainWindow(),
                   UIGlobals::GetDialogLook(),
                   _("System service"),
                   job))
      throw std::runtime_error{"Failed to start system service job"};

    Refresh(fields);
  } catch (...) {
    ShowError(std::current_exception(), _("System service"));
    Refresh(fields);
  }
  timer.Schedule(refresh_interval);
}

} // namespace

std::unique_ptr<Widget>
CreateSystemdConfigPanel()
{
  auto fields      = std::make_shared<Fields>();
  fields->services = BuildSystemdServiceList();
  fields->statuses.resize(fields->services.size());

  auto timer =
    std::make_shared<UI::PeriodicTimer>([fields] { Refresh(*fields); });

  auto list = std::make_unique<GroupedListWidget>(UIGlobals::GetDialogLook());
  fields->list = list.get();
  list->AddGroup(nullptr);

  if (fields->services.empty()) {
    GroupedListWidget::ItemOptions empty;
    empty.description = _("No selected systemd units are installed.");
    list->AddValue(_("No supported services found"), empty);
  } else {
    for (std::size_t i = 0; i < fields->services.size(); ++i) {
      const auto &service = fields->services[i];

      GroupedListWidget::ItemOptions options;
      options.description    = service.description;
      options.subtitle       = service.unit_name.c_str();
      options.value_callback = [fields,
                                i](GroupedListWidget::ValueState &state) {
        ApplyState(fields->statuses[i], state);
      };
      list->AddValue(
        service.display_name,
        [fields, timer, i] {
          if (!CanToggle(fields->statuses[i])) return;

          const auto action =
            fields->statuses[i].state == Systemd::ActiveState::ACTIVE
              ? SystemdAction::STOP
              : SystemdAction::START;
          RunAction(*fields, *timer, action, fields->services[i].unit_name);
        },
        std::move(options));

      GroupedListWidget::ItemOptions restart;
      restart.value_callback = [fields,
                                i](GroupedListWidget::ValueState &state) {
        const auto &status = fields->statuses[i];
        state.hidden =
          !status.valid || status.state != Systemd::ActiveState::ACTIVE;
      };
      list->AddValue(
        _("Restart"),
        [fields, timer, i] {
          if (i >= fields->statuses.size() || !fields->statuses[i].valid ||
              fields->statuses[i].state != Systemd::ActiveState::ACTIVE)
            return;

          RunAction(*fields,
                    *timer,
                    SystemdAction::RESTART,
                    fields->services[i].unit_name);
        },
        std::move(restart));
    }
  }

  list->SetVisibilityCallback([fields, timer](bool visible) {
    if (visible) {
      Refresh(*fields);
      timer->Schedule(refresh_interval);
    } else
      timer->Cancel();
  });

  return list;
}
