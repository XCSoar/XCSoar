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

#include "AirspaceWarningMonitor.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Asset.hpp"
#include "Audio/Sound.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "Event/Idle.hpp"
#include "PageActions.hpp"
#include "Widget/QuestionWidget.hpp"
#include "Form/ActionListener.hpp"
#include "Language/Language.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Formatter/TimeFormatter.hpp"

class AirspaceWarningWidget final
  : public QuestionWidget, private ActionListener {
  enum Action {
    ACK,
    ACK_DAY,
    MORE,
  };

  AirspaceWarningMonitor &monitor;
  ProtectedAirspaceWarningManager &manager;

  const AbstractAirspace &airspace;
  AirspaceWarning::State state;

  StaticString<256> buffer;

  gcc_pure
  const TCHAR *MakeMessage(const AbstractAirspace &airspace,
                           AirspaceWarning::State state,
                           const AirspaceInterceptSolution &solution) {
    if (state == AirspaceWarning::WARNING_INSIDE)
      buffer.Format(_T("%s: %s"), _("Inside airspace"), airspace.GetName());
    else
      buffer.Format(_T("%s: %s (%s)"), _("Near airspace"), airspace.GetName(),
                    FormatTimespanSmart(int(solution.elapsed_time),
                                        2).c_str());

    return buffer;
  }

public:
  AirspaceWarningWidget(AirspaceWarningMonitor &_monitor,
                        ProtectedAirspaceWarningManager &_manager,
                        const AbstractAirspace &_airspace,
                        AirspaceWarning::State _state,
                        const AirspaceInterceptSolution &solution)
    :QuestionWidget(MakeMessage(_airspace, _state, solution), *this),
     monitor(_monitor), manager(_manager),
     airspace(_airspace), state(_state) {
    AddButton(_("ACK"), ACK);
    AddButton(_("ACK Day"), ACK_DAY);
    AddButton(_("More"), MORE);
  }

  ~AirspaceWarningWidget() {
    assert(monitor.widget == this);
    monitor.widget = nullptr;
  }

  bool Update(const AbstractAirspace &_airspace,
              AirspaceWarning::State _state,
              const AirspaceInterceptSolution &solution) {
    if (&_airspace != &airspace)
      return false;

    state = _state;
    SetMessage(MakeMessage(airspace, state, solution));
    return true;
  }

private:
  /* virtual methods from class ActionListener */
  void OnAction(int id) override;
};

void
AirspaceWarningWidget::OnAction(int id)
{
  switch ((Action)id) {
  case ACK:
    if (state == AirspaceWarning::WARNING_INSIDE)
      manager.AcknowledgeInside(airspace);
    else
      manager.AcknowledgeWarning(airspace);
    monitor.Schedule();
    PageActions::RestoreBottom();
    break;

  case ACK_DAY:
    manager.AcknowledgeDay(airspace);
    monitor.Schedule();
    PageActions::RestoreBottom();
    break;

  case MORE:
    dlgAirspaceWarningsShowModal(manager);
    return;
  }
}

void
AirspaceWarningMonitor::Reset()
{
  const auto &calculated = CommonInterface::Calculated();

  last = calculated.airspace_warnings.latest;
}

void
AirspaceWarningMonitor::HideWidget()
{
  if (widget != nullptr)
    PageActions::RestoreBottom();
  assert(widget == nullptr);
}

void
AirspaceWarningMonitor::Check()
{
  const auto &calculated = CommonInterface::Calculated();

  if (widget == nullptr && calculated.airspace_warnings.latest == last)
    return;

  /* there's a new airspace warning */

  last = calculated.airspace_warnings.latest;

  auto *airspace_warnings = GetAirspaceWarnings();
  if (airspace_warnings == nullptr) {
    HideWidget();
    return;
  }

  if (!HasPointer()) {
    /* "classic" list-only view for devices without touch screen */

    if (dlgAirspaceWarningVisible())
      /* already visible */
      return;

    // un-blank the display, play a sound
    ResetUserIdle();
    PlayResource(_T("IDR_WAV_BEEPBWEEP"));

    // show airspace warnings dialog
    if (CommonInterface::GetUISettings().enable_airspace_warning_dialog)
      dlgAirspaceWarningsShowModal(*airspace_warnings, true);
    return;
  }

  const AbstractAirspace *airspace = nullptr;
  AirspaceWarning::State state;
  AirspaceInterceptSolution solution;

  {
    const ProtectedAirspaceWarningManager::Lease lease(*airspace_warnings);
    auto w = lease->begin();
    if (w != lease->end() && w->IsAckExpired()) {
      airspace = &w->GetAirspace();
      state = w->GetWarningState();
      solution = w->GetSolution();
    }
  }

  if (airspace == nullptr) {
    HideWidget();
    return;
  }

  if (CommonInterface::GetUISettings().enable_airspace_warning_dialog) {
    /* show airspace warning */
    if (widget != nullptr) {
      if (widget->Update(*airspace, state, solution))
        return;

      HideWidget();
    }

    widget = new AirspaceWarningWidget(*this, *airspace_warnings,
                                       *airspace, state, solution);
    PageActions::SetCustomBottom(widget);
  }

  // un-blank the display, play a sound
  ResetUserIdle();
  PlayResource(_T("IDR_WAV_BEEPBWEEP"));
}
