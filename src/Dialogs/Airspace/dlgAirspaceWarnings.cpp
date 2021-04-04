/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "AirspaceWarningDialog.hpp"
#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/Button.hpp"
#include "Look/DialogLook.hpp"
#include "Formatter/UserUnits.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "util/TrivialArray.hxx"
#include "util/Macros.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/ListWidget.hpp"
#include "UIGlobals.hpp"
#include "util/Compiler.h"
#include "Audio/Sound.hpp"

#include <cassert>
#include <stdio.h>

struct WarningItem
{
  const AbstractAirspace *airspace;
  AirspaceWarning::State state;
  AirspaceInterceptSolution solution;
  bool ack_expired, ack_day;

  WarningItem() = default;

  WarningItem(const AirspaceWarning &warning)
    :airspace(&warning.GetAirspace()),
     state(warning.GetWarningState()),
     solution(warning.GetSolution()),
     ack_expired(warning.IsAckExpired()), ack_day(warning.GetAckDay()) {}

  bool operator==(const AbstractAirspace &other) const {
    return &other == airspace;
  }
};

class AirspaceWarningListWidget final
  : public ListWidget {

  ProtectedAirspaceWarningManager &airspace_warnings;

  UI::PeriodicTimer update_list_timer{[this]{ UpdateList(); }};

  Button *ack_button;
  Button *ack_day_button;
  Button *enable_button;

  TrivialArray<WarningItem, 64u> warning_list;

  /**
   * Current list cursor airspace.
   */
  const AbstractAirspace *selected_airspace;

  TwoTextRowsRenderer row_renderer;

  /**
   * Airspace repetitive warning sound interval counter.
   */
  unsigned sound_interval_counter;

public:
  AirspaceWarningListWidget(ProtectedAirspaceWarningManager &aw)
    :airspace_warnings(aw),
     selected_airspace(nullptr),
     sound_interval_counter(1)
  {}

  void CreateButtons(WidgetDialog &buttons) {
    ack_button = buttons.AddButton(_("ACK"), [this](){ Ack(); });
    ack_day_button = buttons.AddButton(_("ACK Day"), [this](){ AckDay(); });
    enable_button = buttons.AddButton(_("Enable"), [this](){ Enable(); });
  }

  void CopyList();
  void UpdateList();
  void UpdateButtons();

  gcc_pure
  const AbstractAirspace *GetSelectedAirspace() const;

  gcc_pure
  bool HasWarning() const;

  void Ack();
  void AckDay();
  void Enable();

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Show(const PixelRect &rc) noexcept override;
  void Hide() noexcept override;

  /* virtual methods from ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from ListCursorHandler */
  void OnCursorMoved(unsigned index) noexcept override;

  bool CanActivateItem(unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override;
};

static WndForm *dialog = NULL;
static AirspaceWarningListWidget *list;

static constexpr Color inside_color(254,50,50);
static constexpr Color near_color(254,254,50);
static constexpr Color inside_ack_color(254,100,100);
static constexpr Color near_ack_color(254,254,100);
static bool auto_close = true;


const AbstractAirspace *
AirspaceWarningListWidget::GetSelectedAirspace() const
{
  return selected_airspace;
}

void
AirspaceWarningListWidget::UpdateButtons()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace == NULL) {
    ack_button->SetVisible(false);
    ack_day_button->SetVisible(false);
    enable_button->SetVisible(false);
    return;
  }

  bool ack_expired, ack_day;

  {
    ProtectedAirspaceWarningManager::ExclusiveLease lease(airspace_warnings);
    const AirspaceWarning &warning = lease->GetWarning(*airspace);
    ack_expired = warning.IsAckExpired();
    ack_day = warning.GetAckDay();
  }

  ack_button->SetVisible(ack_expired);
  ack_day_button->SetVisible(!ack_day);
  enable_button->SetVisible(!ack_expired);
}

void
AirspaceWarningListWidget::Prepare(ContainerWindow &parent,
                                   const PixelRect &rc) noexcept
{
  const auto &look = UIGlobals::GetDialogLook();

  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font, *look.list.font));
}

void
AirspaceWarningListWidget::OnCursorMoved(unsigned i) noexcept
{
  selected_airspace = i < warning_list.size()
    ? warning_list[i].airspace
    : NULL;

  UpdateButtons();
}

void
AirspaceWarningListWidget::Show(const PixelRect &rc) noexcept
{
  sound_interval_counter = 0;
  ListWidget::Show(rc);
  UpdateList();
  update_list_timer.Schedule(std::chrono::milliseconds(500));
}

void
AirspaceWarningListWidget::Hide() noexcept
{
  update_list_timer.Cancel();
  ListWidget::Hide();
}

void
AirspaceWarningListWidget::OnActivateItem(gcc_unused unsigned i) noexcept
{
  if (selected_airspace != nullptr)
    dlgAirspaceDetails(*selected_airspace, &airspace_warnings);
}

bool
AirspaceWarningListWidget::HasWarning() const
{
  ProtectedAirspaceWarningManager::Lease lease(airspace_warnings);
  for (auto i = lease->begin(), end = lease->end(); i != end; ++i)
    if (i->IsAckExpired())
      return true;

  return false;
}

static void
Hide()
{
  dialog->Hide();
  dialog->SetModalResult(mrOK);
}

static void
AutoHide()
{
  // Close the dialog if no warning exists and AutoClose is set
  if (!list->HasWarning() && auto_close)
    Hide();
}

void
AirspaceWarningListWidget::Ack()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings.Acknowledge(*airspace);
    UpdateList();
    AutoHide();
  }
}

void
AirspaceWarningListWidget::AckDay()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings.AcknowledgeDay(*airspace, true);
    UpdateList();
    AutoHide();
  }
}

void
AirspaceWarningListWidget::Enable()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace == NULL)
    return;

  {
    ProtectedAirspaceWarningManager::ExclusiveLease lease(airspace_warnings);
    AirspaceWarning *warning = lease->GetWarningPtr(*airspace);
    if (warning == NULL)
      return;

    warning->AcknowledgeInside(false);
    warning->AcknowledgeWarning(false);
    warning->AcknowledgeDay(false);
  }

  UpdateList();
}

void
AirspaceWarningListWidget::OnPaintItem(Canvas &canvas,
                                       const PixelRect paint_rc,
                                       unsigned i) noexcept
{
  TCHAR buffer[128];

  // This constant defines the margin that should be respected
  // for renderring within the paint_rc area.
  const unsigned padding = Layout::GetTextPadding();

  if (i == 0 && warning_list.empty()) {
    /* the warnings were emptied between the opening of the dialog and
       this refresh, so only need to display "No Warnings" for top
       item, otherwise exit immediately */
    row_renderer.DrawFirstRow(canvas, paint_rc, _("No Warnings"));
    return;
  }

  assert(i < warning_list.size());

  const WarningItem &warning = warning_list[i];
  const AbstractAirspace &airspace = *warning.airspace;
  const AirspaceInterceptSolution &solution = warning.solution;

  // word "inside" is used as the etalon, because it is longer than "near" and
  // currently (9.4.2011) there is no other possibility for the status text.
  const int status_width = canvas.CalcTextWidth(_T("inside"));
  // "1888" is used in order to have enough space for 4-digit heights with "AGL"
  const int altitude_width = canvas.CalcTextWidth(_T("1888 m AGL"));

  // Dynamic columns scaling - "name" column is flexible, altitude and state
  // columns are fixed-width.
  auto [text_altitude_rc, status_rc] =
    paint_rc.VerticalSplit(paint_rc.right - (2 * padding + status_width));
  auto text_rc =
    text_altitude_rc.VerticalSplit(text_altitude_rc.right - (padding + altitude_width)).first;
  text_rc.right -= padding;

  if (!warning.ack_expired)
    canvas.SetTextColor(COLOR_GRAY);

  { // name, altitude info
    StringFormat(buffer, ARRAY_SIZE(buffer), _T("%s %s"),
                 airspace.GetName(),
                 AirspaceFormatter::GetClass(airspace));

    row_renderer.DrawFirstRow(canvas, text_rc, buffer);

    AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetTop());
    row_renderer.DrawRightFirstRow(canvas, text_altitude_rc, buffer);

    AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetBase());
    row_renderer.DrawRightSecondRow(canvas, text_altitude_rc, buffer);
  }

  if (warning.state != AirspaceWarning::WARNING_INSIDE &&
      warning.state > AirspaceWarning::WARNING_CLEAR &&
      solution.IsValid()) {

    _stprintf(buffer, _T("%d secs"),
              (int)solution.elapsed_time);

    if (solution.distance > 0)
      _stprintf(buffer + _tcslen(buffer), _T(" dist %d m"),
                (int)solution.distance);
    else {
      /* the airspace is right above or below us - show the vertical
         distance */
      _tcscat(buffer, _T(" vertical "));

      auto delta = solution.altitude - CommonInterface::Basic().nav_altitude;
      FormatRelativeUserAltitude(delta, buffer + _tcslen(buffer), true);
    }

    row_renderer.DrawSecondRow(canvas, text_rc, buffer);
  }

  /* draw the warning state indicator */

  Color state_color;
  const TCHAR *state_text;

  if (warning.state == AirspaceWarning::WARNING_INSIDE) {
    state_color = warning.ack_expired ? inside_color : inside_ack_color;
    state_text = _T("inside");
  } else if (warning.state > AirspaceWarning::WARNING_CLEAR) {
    state_color = warning.ack_expired ? near_color : near_ack_color;
    state_text = _T("near");
  } else {
    state_color = COLOR_WHITE;
    state_text = NULL;
  }

  const PixelSize state_text_size =
    canvas.CalcTextSize(state_text != NULL ? state_text : _T("W"));

  if (state_color != COLOR_WHITE) {
    /* colored background */
    PixelRect rc = status_rc;
    rc.top += padding;
    rc.right -= padding;
    rc.bottom -= padding;

    canvas.DrawFilledRectangle(rc, state_color);

    /* on this background we just painted, we must use black color for
       the state text; our caller might have selected a different
       color, override it here */
    canvas.SetTextColor(COLOR_BLACK);
  }

  if (state_text != NULL) {
    // -- status text will be centered inside its table cell:
    canvas.DrawText(status_rc.CenteredTopLeft(state_text_size), state_text);
  }
}

inline void
AirspaceWarningListWidget::CopyList()
{
  const ProtectedAirspaceWarningManager::Lease lease(airspace_warnings);

  warning_list.clear();
  for (auto i = lease->begin(), end = lease->end();
       i != end && !warning_list.full(); ++i)
    warning_list.push_back(*i);
}

void
AirspaceWarningListWidget::UpdateList()
{
  CopyList();

  if (!warning_list.empty()) {
    GetList().SetLength(warning_list.size());

    int i = -1;
    if (selected_airspace != NULL) {
      auto it = std::find(warning_list.begin(), warning_list.end(),
                          *selected_airspace);
      if (it != warning_list.end()) {
        i = it - warning_list.begin();
        GetList().SetCursorIndex(i);
      }
    }

    if (i < 0)
      /* the selection may have changed, update CursorAirspace */
      OnCursorMoved(GetList().GetCursorIndex());

    // Process repetitive sound warnings if they are enabled in config
    const AirspaceWarningConfig &warning_config =
      CommonInterface::GetComputerSettings().airspace.warnings;
    if (warning_config.repetitive_sound) {
      unsigned tt_closest_airspace = 1000;
      for (auto i : warning_list) {
        /* Find smallest time to nearest aispace (cannot always rely
           on fact that closest airspace should be in the beginning of
           the list) */
        if (i.state < AirspaceWarning::WARNING_INSIDE)
          tt_closest_airspace = std::min(tt_closest_airspace,
                                         unsigned(i.solution.elapsed_time));
        else
          tt_closest_airspace = 0;
      }

      const unsigned sound_interval =
        ((tt_closest_airspace * 3 / warning_config.warning_time) + 1) * 2;
      if (sound_interval_counter >= sound_interval) {
        PlayResource(_T("IDR_WAV_BEEPBWEEP"));
        sound_interval_counter = 1;
      } else
        ++sound_interval_counter;
    }
  } else {
    GetList().SetLength(1);
    selected_airspace = NULL;
    sound_interval_counter = 0;
  }

  GetList().Invalidate();
  UpdateButtons();
  AutoHide();
}

bool
dlgAirspaceWarningVisible()
{
  return (dialog != NULL);
}

void
dlgAirspaceWarningsShowModal(ProtectedAirspaceWarningManager &_warnings,
                             bool _auto_close)
{
  if (dlgAirspaceWarningVisible())
    return;

  auto_close = _auto_close;

  list = new AirspaceWarningListWidget(_warnings);

  WidgetDialog dialog2(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                       UIGlobals::GetDialogLook(),
                       _("Airspace Warnings"), list);
  list->CreateButtons(dialog2);
  dialog2.AddButton(_("Close"), mrOK);
  dialog2.EnableCursorSelection();

  dialog = &dialog2;

  dialog2.ShowModal();

  // Needed for dlgAirspaceWarningVisible()
  dialog = NULL;
}
