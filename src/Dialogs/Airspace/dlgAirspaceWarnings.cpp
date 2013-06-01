/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/Button.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Event/Timer.hpp"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Util/TrivialArray.hpp"
#include "Util/Macros.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widget/DockWindow.hpp"
#include "Widget/ListWidget.hpp"
#include "UIGlobals.hpp"
#include "Compiler.h"

#include <assert.h>
#include <stdlib.h>
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
  : public ListWidget, private Timer {
  ProtectedAirspaceWarningManager &airspace_warnings;

  WndButton *ack_warn_button;
  WndButton *ack_day_button;
  WndButton *ack_space_button;
  WndButton *enable_button;

  TrivialArray<WarningItem, 64u> warning_list;

  /**
   * Current list cursor airspace.
   */
  const AbstractAirspace *selected_airspace;

  /**
   * Current action airspace.
   */
  const AbstractAirspace *focused_airspace;

public:
  AirspaceWarningListWidget(ProtectedAirspaceWarningManager &aw, SubForm &form)
    :airspace_warnings(aw),
     ack_warn_button((WndButton *)form.FindByName(_T("frmAck1"))),
     ack_day_button((WndButton *)form.FindByName(_T("frmAck2"))),
     ack_space_button((WndButton *)form.FindByName(_T("frmAck"))),
     enable_button((WndButton *)form.FindByName(_T("frmEnable"))),
     selected_airspace(nullptr), focused_airspace(nullptr)
  {}

  void CopyList();
  void UpdateList();
  void UpdateButtons();

  gcc_pure
  const AbstractAirspace *GetSelectedAirspace() const;

  gcc_pure
  bool HasWarning() const;

  void AckInside();
  void AckWarning();
  void AckDay();
  void Enable();

  /* virtual methods from Widget */
  virtual void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual void Unprepare() override {
    DeleteWindow();
  }
  virtual void Show(const PixelRect &rc) override;
  virtual void Hide() override;
  virtual bool KeyPress(unsigned key_code) override;

  /* virtual methods from ListItemRenderer */
  virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
                           unsigned idx) override;

  /* virtual methods from ListCursorHandler */
  virtual void OnCursorMoved(unsigned index) override;

  virtual bool CanActivateItem(unsigned index) const override {
    return true;
  }

  virtual void OnActivateItem(unsigned index) override;

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
  return HasPointer() || focused_airspace == NULL
    ? selected_airspace
    : focused_airspace;
}

void
AirspaceWarningListWidget::UpdateButtons()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace == NULL) {
    ack_warn_button->SetVisible(false);
    ack_day_button->SetVisible(false);
    ack_space_button->SetVisible(false);
    enable_button->SetVisible(false);
    return;
  }

  bool ack_expired, ack_day, inside;

  {
    ProtectedAirspaceWarningManager::ExclusiveLease lease(airspace_warnings);
    const AirspaceWarning &warning = lease->GetWarning(*airspace);
    ack_expired = warning.IsAckExpired();
    ack_day = warning.GetAckDay();
    inside = warning.GetWarningState() == AirspaceWarning::WARNING_INSIDE;
  }

  ack_warn_button->SetVisible(ack_expired && !inside);
  ack_day_button->SetVisible(!ack_day);
  ack_space_button->SetVisible(ack_expired && inside);
  enable_button->SetVisible(!ack_expired);
}

void
AirspaceWarningListWidget::Prepare(ContainerWindow &parent,
                                   const PixelRect &rc)
{
  CreateList(parent, UIGlobals::GetDialogLook(), rc,
             Layout::Scale(30));
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
  if (!HasPointer())
    /* on platforms without a pointing device (e.g. ALTAIR), allow
       "focusing" an airspace by pressing enter */
    focused_airspace = selected_airspace;
  else if (selected_airspace != NULL)
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
AirspaceWarningListWidget::AckInside()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings.AcknowledgeInside(*airspace, true);
    UpdateList();
    AutoHide();
  }
}

static void
OnAckClicked()
{
  list->AckInside();
}

void
AirspaceWarningListWidget::AckWarning()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings.AcknowledgeWarning(*airspace, true);
    UpdateList();
    AutoHide();
  }
}

static void
OnAck1Clicked()
{
  list->AckWarning();
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

static void
OnAck2Clicked()
{
  list->AckDay();
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

static void
OnEnableClicked()
{
  list->Enable();
}

static void
OnCloseClicked()
{
  Hide();
}

bool
AirspaceWarningListWidget::KeyPress(unsigned key_code)
{
  switch (key_code){
    case KEY_ESCAPE:
      ::Hide();
    return true;

#ifdef GNAV
    case KEY_APP1:
    case '6':
      AckWarning();
    return true;

    case KEY_APP2:
    case '7':
      AckInside();
    return true;

    case KEY_APP3:
    case '8':
      AckDay();
    return true;

    case KEY_APP4:
    case '9':
      Enable();
    return true;
#endif

  default:
    return false;
  }
}

void
AirspaceWarningListWidget::OnPaintItem(Canvas &canvas,
                                       const PixelRect paint_rc, unsigned i)
{
  TCHAR buffer[128];

  // This constant defines the margin that should be respected
  // for renderring within the paint_rc area.
  const int padding = 2;

  if (i == 0 && warning_list.empty()) {
    /* the warnings were emptied between the opening of the dialog and
       this refresh, so only need to display "No Warnings" for top
       item, otherwise exit immediately */
    canvas.DrawText(paint_rc.left + Layout::Scale(padding),
                    paint_rc.top + Layout::Scale(padding), _("No Warnings"));
    return;
  }

  assert(i < warning_list.size());

  const WarningItem &warning = warning_list[i];
  const AbstractAirspace &airspace = *warning.airspace;
  const AirspaceInterceptSolution &solution = warning.solution;

  const UPixelScalar text_height = 12, text_top = 1;

  // word "inside" is used as the etalon, because it is longer than "near" and
  // currently (9.4.2011) there is no other possibility for the status text.
  const int status_width = canvas.CalcTextWidth(_T("inside"));
  // "1888" is used in order to have enough space for 4-digit heights with "AGL"
  const int altitude_width = canvas.CalcTextWidth(_T("1888 m AGL"));

  // Dynamic columns scaling - "name" column is flexible, altitude and state
  // columns are fixed-width.
  const PixelScalar left0 = Layout::FastScale(padding),
    left2 = paint_rc.right - Layout::FastScale(padding) - (status_width + 2 * Layout::FastScale(padding)),
    left1 = left2 - Layout::FastScale(padding) - altitude_width;

  PixelRect rc_text_clip = paint_rc;
  rc_text_clip.right = left1 - Layout::FastScale(padding);

  if (!warning.ack_expired)
    canvas.SetTextColor(COLOR_GRAY);

  { // name, altitude info
    StringFormat(buffer, ARRAY_SIZE(buffer), _T("%s %s"),
                 airspace.GetName(),
                 AirspaceFormatter::GetClass(airspace));

    canvas.DrawClippedText(paint_rc.left + left0,
                           paint_rc.top + Layout::Scale(text_top),
                           rc_text_clip, buffer);

    AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetTop());
    canvas.DrawText(paint_rc.left + left1,
                    paint_rc.top + Layout::Scale(text_top), buffer);

    AirspaceFormatter::FormatAltitudeShort(buffer, airspace.GetBase());
    canvas.DrawText(paint_rc.left + left1,
                    paint_rc.top + Layout::Scale(text_top + text_height),
                    buffer);
  }

  if (warning.state != AirspaceWarning::WARNING_INSIDE &&
      warning.state > AirspaceWarning::WARNING_CLEAR &&
      solution.IsValid()) {

    _stprintf(buffer, _T("%d secs"),
              (int)solution.elapsed_time);

    if (positive(solution.distance))
      _stprintf(buffer + _tcslen(buffer), _T(" dist %d m"),
                (int)solution.distance);
    else {
      /* the airspace is right above or below us - show the vertical
         distance */
      _tcscat(buffer, _T(" vertical "));

      fixed delta = solution.altitude - CommonInterface::Basic().nav_altitude;
      FormatRelativeUserAltitude(delta, buffer + _tcslen(buffer), true);
    }

    canvas.DrawClippedText(paint_rc.left + left0,
                           paint_rc.top + Layout::Scale(text_top + text_height),
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
    rc.top = paint_rc.top + Layout::FastScale(padding);
    rc.right = paint_rc.right - Layout::FastScale(padding);
    rc.bottom = paint_rc.bottom - Layout::FastScale(padding);

    canvas.DrawFilledRectangle(rc, state_color);

    /* on this background we just painted, we must use black color for
       the state text; our caller might have selected a different
       color, override it here */
    canvas.SetTextColor(COLOR_BLACK);
  }

  if (state_text != NULL) {
    // -- status text will be centered inside its table cell:
    canvas.DrawText(paint_rc.left + left2 + Layout::FastScale(padding) + (status_width / 2)  - (canvas.CalcTextWidth(state_text) / 2),
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
  } else {
    GetList().SetLength(1);
    selected_airspace = NULL;
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

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnAckClicked),
  DeclareCallBackEntry(OnAck1Clicked),
  DeclareCallBackEntry(OnAck2Clicked),
  DeclareCallBackEntry(OnEnableClicked),
  DeclareCallBackEntry(OnCloseClicked),
  DeclareCallBackEntry(NULL)
};

void
dlgAirspaceWarningsShowModal(SingleWindow &parent,
                             ProtectedAirspaceWarningManager &_warnings,
                             bool _auto_close)
{
  if (dlgAirspaceWarningVisible())
    return;

  dialog = LoadDialog(CallBackTable, parent, _T("IDR_XML_AIRSPACEWARNINGS"));
  assert(dialog != NULL);

  DockWindow *dock = (DockWindow *)dialog->FindByName(_T("list"));
  assert(dock != nullptr);

  list = new AirspaceWarningListWidget(_warnings, *dialog);
  dock->SetWidget(list);

  auto_close = _auto_close;

  dialog->ShowModal();

  delete dialog;

  // Needed for dlgAirspaceWarningVisible()
  dialog = NULL;
}
