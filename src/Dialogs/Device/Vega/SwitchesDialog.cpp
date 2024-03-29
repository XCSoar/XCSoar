// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SwitchesDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "Form/DataField/Enum.hpp"

static constexpr StaticEnumChoice flap_position_list[] = {
  { SwitchState::FlapPosition::UNKNOWN, N_("Unknown") },
  { SwitchState::FlapPosition::POSITIVE, N_("Positive") },
  { SwitchState::FlapPosition::NEUTRAL, N_("Neutral") },
  { SwitchState::FlapPosition::NEGATIVE, N_("Negative") },
  { SwitchState::FlapPosition::LANDING, N_("Landing") },
  nullptr
};

static constexpr StaticEnumChoice user_switch_list[] = {
  { SwitchState::UserSwitch::UNKNOWN, N_("Unknown") },
  { SwitchState::UserSwitch::UP, N_("Up") },
  { SwitchState::UserSwitch::MIDDLE, N_("Middle") },
  { SwitchState::UserSwitch::DOWN, N_("Down") },
  nullptr
};

static constexpr StaticEnumChoice airbrake_state_list[] = {
  { SwitchState::AirbrakeState::UNKNOWN, N_("Unknown") },
  { SwitchState::AirbrakeState::LOCKED, N_("Locked") },
  { SwitchState::AirbrakeState::NOT_LOCKED, N_("Not locked") },
  nullptr
};

class SwitchesLeft : public RowFormWidget {
  enum Controls {
    AIRBRAKE_STATE,
    FLAP_POSITION,
    GEAR_DOWN,
    ACKNOWLEDGE,
    REPEAT,
    SPEED_COMMAND,
  };

public:
  SwitchesLeft(const DialogLook &look):RowFormWidget(look) {}

  void Create() {
    AddEnum(_("Airbrake locked"), nullptr, airbrake_state_list, false);
    AddEnum(_("Flaps"), nullptr, flap_position_list, 0);
    AddBoolean(_("Gear down"), nullptr, false);
    AddBoolean(_("Acknowledge"), nullptr, false);
    AddBoolean(_("Repeat"), nullptr, false);
    AddBoolean(_("Speed command"), nullptr, false);
  }

  void Update(const SwitchState &switches) {
    LoadValueEnum(AIRBRAKE_STATE, switches.airbrake_state);
    LoadValueEnum(FLAP_POSITION, (unsigned)switches.flap_position);
    LoadValue(GEAR_DOWN, switches.vega.GetGearExtended());
    LoadValue(ACKNOWLEDGE, switches.vega.GetAcknowledge());
    LoadValue(SPEED_COMMAND, switches.vega.GetSpeedCommand());
  }
};

class SwitchesRight : public RowFormWidget {
  enum Controls {
    USER_SWITCH,
    FLIGHT_MODE,
  };

public:
  SwitchesRight(const DialogLook &look):RowFormWidget(look) {}

  void Create() {
    AddEnum(_("Switch"), nullptr, user_switch_list, false);
    AddBoolean(_("Vario circling"), nullptr, false);
  }

  void Update(const SwitchState &switches) {
    LoadValueEnum(USER_SWITCH, switches.user_switch);
    LoadValue(FLIGHT_MODE,
              switches.flight_mode == SwitchState::FlightMode::CIRCLING);
  }
};

class SwitchesDialog : public TwoWidgets, private NullBlackboardListener {
public:
  SwitchesDialog(const DialogLook &look)
    :TwoWidgets(std::make_unique<SwitchesLeft>(look),
                std::make_unique<SwitchesRight>(look),
                false) {}

  void Update(const SwitchState &switches) {
    ((SwitchesLeft &)GetFirst()).Update(switches);
    ((SwitchesRight &)GetSecond()).Update(switches);
  }

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override {
    ((SwitchesLeft &)GetFirst()).Create();
    ((SwitchesRight &)GetSecond()).Create();

    TwoWidgets::Prepare(parent, rc);
  }

  void Show(const PixelRect &rc) noexcept override {
    Update(CommonInterface::Basic().switch_state);
    TwoWidgets::Show(rc);
    CommonInterface::GetLiveBlackboard().AddListener(*this);
  }

  void Hide() noexcept override {
    CommonInterface::GetLiveBlackboard().RemoveListener(*this);
    TwoWidgets::Hide();
  }

private:
  /* virtual methods from BlackboardListener */
  virtual void OnGPSUpdate(const MoreData &basic) override {
    Update(basic.switch_state);
  }
};

void
dlgSwitchesShowModal()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look, _("Switches"),
                      new SwitchesDialog(look));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
