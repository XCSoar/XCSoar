/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
  { (unsigned)SwitchState::FlapPosition::UNKNOWN, N_("Unknown") },
  { (unsigned)SwitchState::FlapPosition::POSITIVE, N_("Positive") },
  { (unsigned)SwitchState::FlapPosition::NEUTRAL, N_("Neutral") },
  { (unsigned)SwitchState::FlapPosition::NEGATIVE, N_("Negative") },
  { (unsigned)SwitchState::FlapPosition::LANDING, N_("Landing") },
  { 0 }
};

static constexpr StaticEnumChoice user_switch_list[] = {
  { (unsigned)SwitchState::UserSwitch::UNKNOWN, N_("Unknown") },
  { (unsigned)SwitchState::UserSwitch::UP, N_("Up") },
  { (unsigned)SwitchState::UserSwitch::MIDDLE, N_("Middle") },
  { (unsigned)SwitchState::UserSwitch::DOWN, N_("Down") },
  { 0 }
};

static constexpr StaticEnumChoice airbrake_state_list[] = {
  { (unsigned)SwitchState::AirbrakeState::UNKNOWN, N_("Unknown") },
  { (unsigned)SwitchState::AirbrakeState::LOCKED, N_("Locked") },
  { (unsigned)SwitchState::AirbrakeState::NOT_LOCKED, N_("Not locked") },
  { 0 }
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
    :TwoWidgets(new SwitchesLeft(look), new SwitchesRight(look), false) {}

  void Update(const SwitchState &switches) {
    ((SwitchesLeft &)GetFirst()).Update(switches);
    ((SwitchesRight &)GetSecond()).Update(switches);
  }

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override {
    ((SwitchesLeft &)GetFirst()).Create();
    ((SwitchesRight &)GetSecond()).Create();

    TwoWidgets::Prepare(parent, rc);
  }

  virtual void Show(const PixelRect &rc) override {
    Update(CommonInterface::Basic().switch_state);
    TwoWidgets::Show(rc);
    CommonInterface::GetLiveBlackboard().AddListener(*this);
  }

  virtual void Hide() override {
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
  WidgetDialog dialog(look);
  dialog.CreateAuto(UIGlobals::GetMainWindow(), _("Switches"),
                    new SwitchesDialog(look));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
