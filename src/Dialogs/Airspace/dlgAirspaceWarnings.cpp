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

#include "AirspaceWarningDialog.hpp"
#include "Airspace.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/Button.hpp"
#include "Look/DialogLook.hpp"
#include "Formatter/UserUnits.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Event/Timer.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Util/TrivialArray.hxx"
#include "Util/Macros.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/ListWidget.hpp"
#include "UIGlobals.hpp"
#include "Compiler.h"
#include "Audio/Sound.hpp"

#include <assert.h>
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
  : public ListWidget, private ActionListener, private Timer {

  enum Buttons {
    ACK,
    ACK_DAY,
    ENABLE,
  };

  ProtectedAirspaceWarningManager &airspace_warnings;

  Button *ack_button;
  Button *ack_day_button;
  Button *enable_button;

  TrivialArray<WarningItem, 64u> warning_list;

  /**
   * Current list cursor airspace.
   */
  const AbstractAirspace *selected_airspace;

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
    ack_button = buttons.AddButton(_("ACK"), *this, ACK);
    ack_day_button = buttons.AddButton(_("ACK Day"), *this, ACK_DAY);
    enable_button = buttons.AddButton(_("Enable"), *this, ENABLE);
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
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual void Unprepare() override {
    DeleteWindow();
  }
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;

  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  virtual void OnCursorMoved(unsigned index) override;

  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override;

private:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;

  /* virtual methods from Timer */
  virtual void OnTimer() override;
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
                                   const PixelRect &rc)
{
  const auto &look = UIGlobals::GetDialogLook();

  const unsigned padding = Layout::GetTextPadding();
  const unsigned font_height = look.list.font->GetHeight();
  const unsigned row_height = 3 * padding + 2 * font_height;

  CreateList(parent, look, rc,
             std::max(Layout::GetMaximumControlHeight(), row_height));
}

void
AirspaceWarningListWidget::OnCursorMoved(unsigned i)
{
  selected_airspace = i < warning_list.size()
    ? warning_list[i].airspace
    : NULL;

  UpdateButtons();
}

void
AirspaceWarningListWidget::Show(const PixelRect &rc)
{
  sound_interval_counter = 0;
  ListWidget::Show(rc);
  UpdateList();
  Timer::Schedule(500);
}

void
AirspaceWarningListWidget::Hide()
{
  Timer::Cancel();
  ListWidget::Hide();
}

void
AirspaceWarningListWidget::OnActivateItem(gcc_unused unsigned i)
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
                                       const PixelRect paint_rc, unsigned i)
{
  TCHAR buffer[128];

  // This constant defines the margin that should be respected
  // for renderring within the paint_rc area.
  const unsigned padding = Layout::GetTextPadding();

  if (i == 0 && warning_list.empty()) {
    /* the warnings were emptied between the opening of the dialog and
       this refresh, so only need to display "No Warnings" for top
       item, otherwise exit immediately */
    canvas.DrawText(paint_rc.left + padding,
                    paint_rc.top + padding, _("No Warnings"));
    return;
  }

  assert(i < warning_list.size());

  const WarningItem &warning = warning_list[i];
  const AbstractAirspace &airspace = *warning.airspace;
  const AirspaceInterceptSolution &solution = warning.solution;

  const unsigned text_height = canvas.GetFontHeight();
  const int first_row_y = paint_rc.top + padding;
  const int second_row_y = first_row_y + text_height + padding;

  // word "inside" is used as the etalon, because it is longer than "near" and
  // currently (9.4.2011) there is no other possibility for the status text.
  const int status_width = canvas.CalcTextWidth(_T("inside"));
  // "1888" is used in order to have enough space for 4-digit heights with "AGL"
  const int altitude_width = canvas.CalcTextWidth(_T("1888 m AGL"));

  // Dynamic columns scaling - "name" column is flexible, altitude and state
  // columns are fixed-width.
  const int left0 = padding,
    left2 = paint_rc.right - padding - (status_width + 2 * padding),
    left1 = left2 - padding - altitude_width;

  PixelRect rc_text_clip = paint_rc;
  rc_text_clip.right = left1 - padding;

  if (!warning.ack_expired)
    canvas.SetTextColor(COLOR_GRAY);

  { // name, altitude info
    StringFormat(buffer, ARRAY_SIZE(buffer), _T("%s %s"),
                 airspace.GetName(),
                 AirspaceFormatter::GetClass(airspace));

    canvas.DrawClippedText(paint_rc.left + left0, first_row_y,
                           rc_text_clip, buffer);

    AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetTop());
    canvas.DrawText(paint_rc.left + left1, first_row_y, buffer);

    AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetBase());
    canvas.DrawText(paint_rc.left + left1, second_row_y, buffer);
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

    canvas.DrawClippedText(paint_rc.left + left0, second_row_y,
                           rc_text_clip, buffer);
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
    PixelRect rc;

    rc.left = paint_rc.left + left2;
    rc.top = paint_rc.top + padding;
    rc.right = paint_rc.right - padding;
    rc.bottom = paint_rc.bottom - padding;

    canvas.DrawFilledRectangle(rc, state_color);

    /* on this background we just painted, we must use black color for
       the state text; our caller might have selected a different
       color, override it here */
    canvas.SetTextColor(COLOR_BLACK);
  }

  if (state_text != NULL) {
    // -- status text will be centered inside its table cell:
    canvas.DrawText(paint_rc.left + left2 + padding + (status_width / 2) - (canvas.CalcTextWidth(state_text) / 2),
                    (paint_rc.bottom + paint_rc.top - state_text_size.cy) / 2,
                    state_text);
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
AirspaceWarningListWidget::OnAction(int id)
{
  switch (id) {
  case ACK:
    Ack();
    break;

  case ACK_DAY:
    AckDay();
    break;

  case ENABLE:
    Enable();
    break;
  }
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

void
AirspaceWarningListWidget::OnTimer()
{
  UpdateList();
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

  WidgetDialog dialog2(UIGlobals::GetDialogLook());
  dialog2.CreateFull(UIGlobals::GetMainWindow(), _("Airspace Warnings"), list);
  list->CreateButtons(dialog2);
  dialog2.AddButton(_("Close"), mrOK);
  dialog2.EnableCursorSelection();

  dialog = &dialog2;

  dialog2.ShowModal();

  // Needed for dlgAirspaceWarningVisible()
  dialog = NULL;
}
