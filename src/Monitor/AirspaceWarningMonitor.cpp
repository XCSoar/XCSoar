// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceWarningMonitor.hpp"
#include "Interface.hpp"
#include "Asset.hpp"
#include "Audio/Sound.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "ui/event/Idle.hpp"
#include "PageActions.hpp"
#include "Widget/QuestionWidget.hpp"
#include "Language/Language.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

class AirspaceWarningWidget final
  : public QuestionWidget {

  AirspaceWarningMonitor &monitor;
  ProtectedAirspaceWarningManager &manager;

  const ConstAirspacePtr airspace;
  AirspaceWarning::State state;

  StaticString<256> buffer;

  [[gnu::pure]]
  const char *MakeMessage(const AbstractAirspace &airspace,
                           AirspaceWarning::State state,
                           const AirspaceInterceptSolution &solution) noexcept {
    if (state == AirspaceWarning::WARNING_INSIDE)
      buffer.Format("%s: %s", _("Inside airspace"), airspace.GetName());
    else
      buffer.Format("%s: %s (%s)", _("Near airspace"), airspace.GetName(),
                    FormatTimespanSmart(solution.elapsed_time,
                                        2).c_str());

    return buffer;
  }

public:
  AirspaceWarningWidget(AirspaceWarningMonitor &_monitor,
                        ProtectedAirspaceWarningManager &_manager,
                        ConstAirspacePtr _airspace,
                        AirspaceWarning::State _state,
                        const AirspaceInterceptSolution &solution) noexcept
    :QuestionWidget(MakeMessage(*_airspace, _state, solution)),
     monitor(_monitor), manager(_manager),
     airspace(std::move(_airspace)), state(_state) {
    AddButton(_("ACK"), [this](){
      if (state == AirspaceWarning::WARNING_INSIDE)
        manager.AcknowledgeInside(airspace);
      else
        manager.AcknowledgeWarning(airspace);
      monitor.Schedule();
      PageActions::RestoreBottom();
    });

    AddButton(_("ACK Day"), [this](){
      manager.AcknowledgeDay(airspace);
      monitor.Schedule();
      PageActions::RestoreBottom();
    });

    AddButton(_("More"), [this](){
      dlgAirspaceWarningsShowModal(manager);
    });
  }

  ~AirspaceWarningWidget() noexcept {
    assert(monitor.widget == this);
    monitor.widget = nullptr;
  }

  bool Update(const AbstractAirspace &_airspace,
              AirspaceWarning::State _state,
              const AirspaceInterceptSolution &solution) noexcept {
    if (&_airspace != airspace.get())
      return false;

    state = _state;
    SetMessage(MakeMessage(*airspace, state, solution));
    return true;
  }
};

void
AirspaceWarningMonitor::Reset() noexcept
{
  const auto &calculated = CommonInterface::Calculated();

  last = calculated.airspace_warnings.latest;
}

void
AirspaceWarningMonitor::HideWidget() noexcept
{
  if (widget != nullptr)
    PageActions::RestoreBottom();
  assert(widget == nullptr);
}

void
AirspaceWarningMonitor::Check() noexcept
{
  const auto &calculated = CommonInterface::Calculated();

  if (widget == nullptr && calculated.airspace_warnings.latest == last)
    return;

  /* there's a new airspace warning */

  last = calculated.airspace_warnings.latest;

  auto *airspace_warnings = backend_components->GetAirspaceWarnings();
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
    PlayResource("IDR_WAV_BEEPBWEEP");

    // show airspace warnings dialog
    if (CommonInterface::GetUISettings().enable_airspace_warning_dialog)
      dlgAirspaceWarningsShowModal(*airspace_warnings, true);
    return;
  }

  const auto w = airspace_warnings->GetTopWarning();

  if (!w || !w->IsActive()) {
    HideWidget();
    return;
  }

  if (CommonInterface::GetUISettings().enable_airspace_warning_dialog) {
    /* show airspace warning */
    if (widget != nullptr) {
      if (widget->Update(w->GetAirspace(), w->GetWarningState(),
                         w->GetSolution()))
        return;

      HideWidget();
    }

    widget = new AirspaceWarningWidget(*this, *airspace_warnings,
                                       w->GetAirspacePtr(),
                                       w->GetWarningState(),
                                       w->GetSolution());
    PageActions::SetCustomBottom(widget);
  }

  // un-blank the display, play a sound
  ResetUserIdle();
  PlayResource("IDR_WAV_BEEPBWEEP");
}
