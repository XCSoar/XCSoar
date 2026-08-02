// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceWarningMonitor.hpp"
#include "Interface.hpp"
#include "Asset.hpp"
#include "Audio/Sound.hpp"
#include "Dialogs/Airspace/AirspaceWarningDialog.hpp"
#include "ui/event/Idle.hpp"
#include "Look/Colors.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/AnyCanvas.hpp"
#include "ui/canvas/Font.hpp"
#include "PageActions.hpp"
#include "Widget/QuestionWidget.hpp"
#include "Language/Language.hpp"
#include "Engine/Airspace/AirspaceWarning.hpp"
#include "Engine/Airspace/AirspaceWarningManager.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Formatter/TimeFormatter.hpp"
#include "Formatter/UserUnits.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "LogFile.hpp"
#include "MainWindow.hpp"
#include "Message.hpp"

#include <exception>

class AirspaceWarningWidget final
  : public QuestionWidget {

  AirspaceWarningMonitor &monitor;
  ProtectedAirspaceWarningManager &manager;

  const ConstAirspacePtr airspace;
  AirspaceWarning::State state;

  /** state and airspace name, e.g. "Near: CTA MARSEILLE 7" */
  StaticString<192> name_text;

  /** distance and time until the intercept; empty while inside */
  StaticString<64> details_text;

  /** the message as shown, composed by Compose() */
  StaticString<256> buffer;

  /** width available for the message text, known from Prepare() */
  unsigned message_width = 0;

  /**
   * True when the name plus the current distance/time do not fit on
   * one line.  Locked after DecideLayout() so later updates (which
   * only shrink those values) cannot change the banner height.
   */
  bool two_lines = false;

  /**
   * Format the two message parts.  The labels match the Inside / Near
   * badges of the airspace warning list.
   */
  void MakeMessage(const AbstractAirspace &airspace,
                   AirspaceWarning::State state,
                   const AirspaceInterceptSolution &solution) noexcept {
    if (state == AirspaceWarning::WARNING_INSIDE) {
      name_text.Format("%s: %s", _("Inside"), airspace.GetName());
      details_text.clear();
      return;
    }

    name_text.Format("%s: %s", _("Near"), airspace.GetName());

    if (solution.distance > 0)
      details_text.Format("%s  ",
                          FormatUserDistanceSmart(solution.distance).c_str());
    else if (solution.IsValid()) {
      /* the airspace is right above or below us, so the intercept has
         no horizontal distance: show the vertical one instead */
      char relative_altitude[32];
      FormatRelativeUserAltitude(solution.altitude
                                 - CommonInterface::Basic().nav_altitude,
                                 relative_altitude);
      details_text.Format("%s  ", relative_altitude);
    } else
      details_text.clear();

    details_text.AppendFormat("%s",
                              FormatTimespanSmart(solution.elapsed_time,
                                                  2).c_str());
  }

  /**
   * The bottom area spans the full width, and WndFrame insets the text
   * by the padding on both sides.
   */
  void SetMessageWidth(const PixelRect &rc) noexcept {
    message_width = rc.GetWidth() - 2 * Layout::GetTextPadding();
  }

  /**
   * Decide one vs two lines from the airspace name and the current
   * distance/time.  Those values shrink as the intercept approaches,
   * so measuring them once (and locking the result) is enough; a
   * synthetic "long" timespan was the wrong shape - FormatTimespanSmart
   * with two tokens is wider for "59 min 59 sec" than for "4 days 3 h".
   */
  void DecideLayout() noexcept {
    if (details_text.empty() || message_width == 0) {
      two_lines = false;
      return;
    }

    AnyCanvas canvas;
    canvas.Select(UIGlobals::GetDialogLook().text_font);

    StaticString<256> one_line;
    one_line.Format("%s  %s", name_text.c_str(), details_text.c_str());
    two_lines = canvas.CalcTextWidth(one_line) > message_width;
  }

  void Compose() noexcept {
    if (details_text.empty())
      buffer = name_text;
    else if (two_lines)
      buffer.Format("%s\n%s", name_text.c_str(), details_text.c_str());
    else
      buffer.Format("%s  %s", name_text.c_str(), details_text.c_str());
  }

  /**
   * Mark the message with the same colours the airspace warning list
   * uses for its Inside / Near badge.
   */
  void UpdateMessageColors() noexcept {
    if (!HasColors())
      /* greyscale and e-paper displays: the plain message keeps more
         contrast than a dithered fill */
      return;

    /* Black on both fills: 5.7:1 on Inside red and 19.4:1 on Near
       yellow, whereas white would only reach 3.7:1 on the red. */
    SetMessageColors(state == AirspaceWarning::WARNING_INSIDE
                     ? COLOR_AIRSPACE_WARNING_INSIDE
                     : COLOR_AIRSPACE_WARNING_NEAR,
                     COLOR_BLACK);
  }

public:
  AirspaceWarningWidget(AirspaceWarningMonitor &_monitor,
                        ProtectedAirspaceWarningManager &_manager,
                        ConstAirspacePtr _airspace,
                        AirspaceWarning::State _state,
                        const AirspaceInterceptSolution &solution) noexcept
    /* the message depends on the width and is only known in
       Prepare() */
    :QuestionWidget(""),
     monitor(_monitor), manager(_manager),
     airspace(std::move(_airspace)), state(_state) {
    MakeMessage(*airspace, state, solution);

    AddButton(_("ACK"), [this](){
      try {
        if (state == AirspaceWarning::WARNING_INSIDE)
          manager.AcknowledgeInside(airspace);
        else
          manager.AcknowledgeWarning(airspace);
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to acknowledge airspace warning");
        Message::AddMessage(_("Failed to acknowledge airspace warning"));
        return;
      }

      monitor.Schedule();
      PageActions::RestoreBottom();
    });

    AddButton(_("Ack Day"), [this](){
      try {
        manager.AcknowledgeDay(airspace);
      } catch (...) {
        LogError(std::current_exception(),
                 "Failed to acknowledge airspace warning for day");
        Message::AddMessage(_("Failed to acknowledge airspace warning for day"));
        return;
      }

      monitor.Schedule();
      PageActions::RestoreBottom();
    });

    AddButton(_("More"), [this](){
      dlgAirspaceWarningsShowModal(manager);
    });

    UpdateMessageColors();
  }

  ~AirspaceWarningWidget() noexcept {
    assert(monitor.widget == this);
    monitor.widget = nullptr;
  }

  /* virtual methods from class Widget */

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override {
    SetMessageWidth(rc);
    DecideLayout();
    Compose();

    QuestionWidget::Prepare(parent, rc);
    SetMessage(buffer);
  }

  void Move(const PixelRect &rc) noexcept override {
    SetMessageWidth(rc);
    DecideLayout();
    Compose();

    QuestionWidget::Move(rc);
    SetMessage(buffer);
  }

  PixelSize GetMinimumSize() const noexcept override {
    /* QuestionWidget already reserves one text line plus the button
       row */
    PixelSize size = QuestionWidget::GetMinimumSize();
    if (two_lines)
      size.height += UIGlobals::GetDialogLook().text_font.GetHeight();
    return size;
  }

  bool Update(const AbstractAirspace &_airspace,
              AirspaceWarning::State _state,
              const AirspaceInterceptSolution &solution) noexcept {
    if (&_airspace != airspace.get())
      return false;

    const bool had_details = !details_text.empty();

    state = _state;
    MakeMessage(*airspace, state, solution);

    /* Inside has no details line; Near may need two.  Recreate when
       that changes so MainWindow picks up the new height. */
    if (had_details != !details_text.empty())
      return false;

    Compose();
    SetMessage(buffer);
    UpdateMessageColors();
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
  if (widget == nullptr)
    return;

  PageActions::RestoreBottom();
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
