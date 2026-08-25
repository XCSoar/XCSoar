// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemdConfigPanel.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/JobDialog.hpp"
#include "Form/Button.hpp"
#include "Form/ButtonPanel.hpp"
#include "Job/Job.hpp"
#include "Language/Language.hpp"
#include "Linux/SystemdServiceList.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "UIGlobals.hpp"
#include "Widget/ButtonPanelWidget.hpp"
#include "Widget/ListWidget.hpp"
#include "lib/dbus/Connection.hxx"
#include "lib/dbus/ScopeMatch.hxx"
#include "lib/dbus/Systemd.hxx"
#include "ui/canvas/Canvas.hpp"
#include "ui/canvas/Color.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "util/ScopeExit.hxx"

#include <chrono>
#include <exception>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

static constexpr auto refresh_interval = std::chrono::seconds{1};
static constexpr int systemd_action_timeout_ms = 30000;

enum class SystemdAction {
  START,
  STOP,
  RESTART,
};

class SystemdActionJob final : public Job {
  const SystemdAction action;
  const std::string unit_name;

public:
  SystemdActionJob(SystemdAction _action, const std::string &_unit_name)
    :action(_action), unit_name(_unit_name) {}

  void Run([[maybe_unused]] OperationEnvironment &env) override {
    auto connection = ODBus::Connection::GetSystemPrivate();
    AtScopeExit(&connection) { connection.Close(); };

    const ODBus::ScopeMatch match{connection, Systemd::job_removed_match};

    switch (action) {
    case SystemdAction::START:
      Systemd::StartUnit(connection, unit_name.c_str(), "replace",
                         systemd_action_timeout_ms);
      Systemd::EnableUnitFile(connection, unit_name.c_str());
      break;

    case SystemdAction::STOP:
      Systemd::StopUnit(connection, unit_name.c_str(), "replace",
                        systemd_action_timeout_ms);
      Systemd::DisableUnitFile(connection, unit_name.c_str());
      break;

    case SystemdAction::RESTART:
      Systemd::RestartUnit(connection, unit_name.c_str(), "replace",
                           systemd_action_timeout_ms);
      break;
    }
  }
};

struct ServiceStatus {
  Systemd::ActiveState state = Systemd::ActiveState::INACTIVE;
  bool valid = false;
};

class SystemdListWidget final : public ListWidget {
  std::vector<SystemdService> services = BuildSystemdServiceList();
  std::vector<ServiceStatus> statuses{services.size()};
  TwoTextRowsRenderer row_renderer;
  Button *toggle_button = nullptr;
  Button *restart_button = nullptr;
  ButtonPanelWidget *button_panel = nullptr;
  UI::PeriodicTimer refresh_timer{[this]{ Refresh(); }};

public:
  void SetButtonPanel(ButtonPanelWidget &_button_panel) noexcept {
    button_panel = &_button_panel;
  }

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    const auto &look = UIGlobals::GetDialogLook();
    CreateList(parent, look, rc,
               row_renderer.CalculateLayout(look.text_font, look.small_font));
    GetList().SetLength(services.empty() ? 1u : services.size());

    if (button_panel != nullptr)
      CreateButtons(button_panel->GetButtonPanel());
  }

  void Show(const PixelRect &rc) noexcept override {
    ListWidget::Show(rc);
    Refresh();
    refresh_timer.Schedule(refresh_interval);
  }

  void Hide() noexcept override {
    refresh_timer.Cancel();
    ListWidget::Hide();
  }

  void Unprepare() noexcept override {
    refresh_timer.Cancel();
    ListWidget::Unprepare();
  }

  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override {
    if (idx >= services.size()) {
      if (services.empty() && idx == 0) {
        row_renderer.DrawFirstRow(canvas, rc,
                                  _("No supported services found"));
        row_renderer.DrawSecondRow(
          canvas, rc, _("No selected systemd units are installed."));
      }

      return;
    }

    const auto &service = services[idx];
    const auto &status = statuses[idx];

    row_renderer.DrawFirstRow(canvas, rc, service.display_name);

    const char *state_text = _("Unavailable");
    Color state_color = COLOR_RED;
    if (status.valid) {
      switch (status.state) {
      case Systemd::ActiveState::ACTIVE:
        state_text = _("On");
        state_color = COLOR_GREEN;
        break;
      case Systemd::ActiveState::INACTIVE:
        state_text = _("Off");
        state_color = canvas.GetTextColor();
        break;
      case Systemd::ActiveState::ACTIVATING:
        state_text = _("Starting...");
        state_color = COLOR_ORANGE;
        break;
      case Systemd::ActiveState::DEACTIVATING:
        state_text = _("Stopping...");
        state_color = COLOR_ORANGE;
        break;
      case Systemd::ActiveState::RELOADING:
        state_text = _("Reloading...");
        state_color = COLOR_ORANGE;
        break;
      case Systemd::ActiveState::FAILED:
        state_text = _("Failed");
        state_color = COLOR_RED;
        break;
      }
    }

    const auto old_color = canvas.GetTextColor();
    canvas.SetTextColor(state_color);
    row_renderer.DrawRightFirstRow(canvas, rc, state_text);
    canvas.SetTextColor(old_color);
    row_renderer.DrawSecondRow(canvas, rc, service.description);
    row_renderer.DrawRightSecondRow(canvas, rc, service.unit_name.c_str());
  }

  void OnCursorMoved([[maybe_unused]] unsigned index) noexcept override {
    UpdateButtons();
  }

  bool CanActivateItem(unsigned index) const noexcept override {
    return CanToggle(index);
  }

  void OnActivateItem(unsigned index) noexcept override {
    Toggle(index);
  }

private:
  void CreateButtons(ButtonPanel &buttons) noexcept {
    toggle_button = buttons.Add(_("Turn on"), [this]{
      Toggle(GetList().GetCursorIndex());
    });
    restart_button = buttons.Add(_("Restart"), [this]{
      Restart(GetList().GetCursorIndex());
    });

    if (!services.empty())
      buttons.EnableCursorSelection();

    UpdateButtons();
  }

  [[gnu::pure]] bool CanToggle(unsigned index) const noexcept {
    if (index >= statuses.size() || !statuses[index].valid)
      return false;

    switch (statuses[index].state) {
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

  void UpdateButtons() noexcept {
    const auto index = GetList().GetCursorIndex();
    const bool can_toggle = CanToggle(index);
    const bool is_active = index < statuses.size() && statuses[index].valid &&
      statuses[index].state == Systemd::ActiveState::ACTIVE;

    if (toggle_button != nullptr) {
      toggle_button->SetCaption(is_active ? _("Turn off") : _("Turn on"));
      toggle_button->SetEnabled(can_toggle);
    }

    if (restart_button != nullptr)
      restart_button->SetEnabled(is_active);

    /* Moving between an active and an inactive unit can disable the
       currently selected Restart action.  Keep cursor-key selection on an
       operable action so Up/Down list navigation and Left/Right action
       selection continue to work together. */
    if (can_toggle && button_panel != nullptr)
      button_panel->GetButtonPanel().ReselectToFirstEnabled();
  }

  void Refresh() noexcept {
    try {
      auto connection = ODBus::Connection::GetSystem();
      for (std::size_t i = 0; i < services.size(); ++i) {
        try {
          statuses[i].state = Systemd::GetUnitActiveState(
            connection, services[i].unit_name.c_str());
          statuses[i].valid = true;
        } catch (...) {
          statuses[i].valid = false;
        }
      }
    } catch (...) {
      for (auto &status : statuses)
        status.valid = false;
    }

    GetList().Invalidate();
    UpdateButtons();
  }

  void Toggle(unsigned index) noexcept {
    if (!CanToggle(index))
      return;

    refresh_timer.Cancel();
    try {
      const auto &unit = services[index].unit_name;
      const auto action = statuses[index].state == Systemd::ActiveState::ACTIVE
        ? SystemdAction::STOP
        : SystemdAction::START;
      SystemdActionJob job{action, unit};

      if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                     _("System service"), job))
        throw std::runtime_error{"Failed to start system service job"};

      Refresh();
    } catch (...) {
      ShowError(std::current_exception(), _("System service"));
      Refresh();
    }
    refresh_timer.Schedule(refresh_interval);
  }

  void Restart(unsigned index) noexcept {
    if (index >= statuses.size() || !statuses[index].valid ||
        statuses[index].state != Systemd::ActiveState::ACTIVE)
      return;

    refresh_timer.Cancel();
    try {
      SystemdActionJob job{SystemdAction::RESTART,
                           services[index].unit_name};
      if (!JobDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                     _("System service"), job))
        throw std::runtime_error{"Failed to start system service job"};

      Refresh();
    } catch (...) {
      ShowError(std::current_exception(), _("System service"));
      Refresh();
    }
    refresh_timer.Schedule(refresh_interval);
  }
};

} // namespace

std::unique_ptr<Widget>
CreateSystemdConfigPanel()
{
  auto list = std::make_unique<SystemdListWidget>();
  auto panel = std::make_unique<ButtonPanelWidget>(
    std::move(list), ButtonPanelWidget::Alignment::BOTTOM);
  static_cast<SystemdListWidget &>(panel->GetWidget()).SetButtonPanel(*panel);
  return panel;
}
