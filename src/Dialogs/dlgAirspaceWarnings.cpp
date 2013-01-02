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

#include "Dialogs/AirspaceWarningDialog.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Airspace.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Form/List.hpp"
#include "Form/Button.hpp"
#include "Units/Units.hpp"
#include "Formatter/UserUnits.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Airspace/AirspaceWarning.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Airspace/AirspaceWarningManager.hpp"
#include "Formatter/AirspaceFormatter.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Util/TrivialArray.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"

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

typedef TrivialArray<WarningItem, 64u> WarningList;

static ProtectedAirspaceWarningManager *airspace_warnings;
static WndForm *dialog = NULL;
static WndButton *ack_warn_button = NULL;
static WndButton *ack_day_button = NULL;
static WndButton *ack_space_button = NULL;
static WndButton *enable_button = NULL; // Enable

static ListControl *warning_list_frame = NULL;
static constexpr Color inside_color(254,50,50);
static constexpr Color near_color(254,254,50);
static constexpr Color inside_ack_color(254,100,100);
static constexpr Color near_ack_color(254,254,100);
static bool auto_close = true;

static WarningList warning_list;

static const AbstractAirspace* selected_airspace = NULL; // Current list cursor airspace
static const AbstractAirspace* focused_airspace = NULL;  // Current action airspace

static const AbstractAirspace *
GetSelectedAirspace()
{
  return HasPointer() || focused_airspace == NULL
    ? selected_airspace
    : focused_airspace;
}

static void
UpdateButtons()
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
    ProtectedAirspaceWarningManager::ExclusiveLease lease(*airspace_warnings);
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

static void
UpdateList();

static void
AirspaceWarningCursorCallback(unsigned i)
{
  selected_airspace = i < warning_list.size()
    ? warning_list[i].airspace
    : NULL;

  UpdateButtons();
}

static void
OnAirspaceListEnter(gcc_unused unsigned i)
{
  if (!HasPointer())
    /* on platforms without a pointing device (e.g. ALTAIR), allow
       "focusing" an airspace by pressing enter */
    focused_airspace = selected_airspace;
  else if (selected_airspace != NULL)
    dlgAirspaceDetails(*selected_airspace, airspace_warnings);
}

static bool
HasWarning()
{
  ProtectedAirspaceWarningManager::Lease lease(*airspace_warnings);
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
  if (!HasWarning() && auto_close)
    Hide();
}

static void
AckInside()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings->acknowledge_inside(*airspace, true);
    UpdateList();
    AutoHide();
  }
}

static void
OnAckClicked(gcc_unused WndButton &Sender)
{
  AckInside();
}

static void
AckWarning()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings->acknowledge_warning(*airspace, true);
    UpdateList();
    AutoHide();
  }
}

static void
OnAck1Clicked(gcc_unused WndButton &Sender)
{
  AckWarning();
}

static void
AckDay()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace != NULL) {
    airspace_warnings->acknowledge_day(*airspace, true);
    UpdateList();
    AutoHide();
  }
}

static void
OnAck2Clicked(gcc_unused WndButton &Sender)
{
  AckDay();
}

static void
Enable()
{
  const AbstractAirspace *airspace = GetSelectedAirspace();
  if (airspace == NULL)
    return;

  {
    ProtectedAirspaceWarningManager::ExclusiveLease lease(*airspace_warnings);
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
OnEnableClicked(gcc_unused WndButton &Sender)
{
  Enable();
}

static void
OnCloseClicked(gcc_unused WndButton &Sender)
{
  Hide();
}

static bool
OnKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code){
    case VK_ESCAPE:
      Hide();
    return true;

#ifdef GNAV
    case VK_APP1:
    case '6':
      AckWarning();
    return true;

    case VK_APP2:
    case '7':
      AckInside();
    return true;

    case VK_APP3:
    case '8':
      AckDay();
    return true;

    case VK_APP4:
    case '9':
      Enable();
    return true;
#endif

  default:
    return false;
  }
}

static void
OnAirspaceListItemPaint(Canvas &canvas, const PixelRect paint_rc, unsigned i)
{
  TCHAR buffer[128];

  // This constant defines the margin that should be respected
  // for renderring within the paint_rc area.
  const int padding = 2;

  if (i == 0 && warning_list.empty()) {
    /* the warnings were emptied between the opening of the dialog and
       this refresh, so only need to display "No Warnings" for top
       item, otherwise exit immediately */
    canvas.text(paint_rc.left + Layout::Scale(padding),
                paint_rc.top + Layout::Scale(padding), _("No Warnings"));
    return;
  }

  assert(i < warning_list.size());

  const WarningItem &warning = warning_list[i];
  const AbstractAirspace &airspace = *warning.airspace;
  const AirspaceInterceptSolution &solution = warning.solution;

  tstring name = AirspaceFormatter::GetNameAndClass(airspace);
  tstring top = AirspaceFormatter::GetTopShort(airspace);
  tstring base = AirspaceFormatter::GetBaseShort(airspace);

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
    _stprintf(buffer, _T("%-20s"), name.c_str());

    canvas.text_clipped(paint_rc.left + left0,
                        paint_rc.top + Layout::Scale(text_top),
                        rc_text_clip, buffer);

    _stprintf(buffer, _T("%-20s"), top.c_str());
    canvas.text(paint_rc.left + left1,
                paint_rc.top + Layout::Scale(text_top), buffer);

    _stprintf(buffer, _T("%-20s"), base.c_str());
    canvas.text(paint_rc.left + left1,
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

    canvas.text_clipped(paint_rc.left + left0,
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
    canvas.text(paint_rc.left + left2 + Layout::FastScale(padding) + (status_width / 2)  - (canvas.CalcTextWidth(state_text) / 2),
                (paint_rc.bottom + paint_rc.top - state_text_size.cy) / 2,
                state_text);
  }
}

static void
CopyList()
{
  const ProtectedAirspaceWarningManager::Lease lease(*airspace_warnings);

  warning_list.clear();
  for (auto i = lease->begin(), end = lease->end();
       i != end && !warning_list.full(); ++i)
    warning_list.push_back(*i);
}

static void
UpdateList()
{
  CopyList();

  if (!warning_list.empty()) {
    warning_list_frame->SetLength(warning_list.size());

    int i = -1;
    if (selected_airspace != NULL) {
      auto it = std::find(warning_list.begin(), warning_list.end(),
                          *selected_airspace);
      if (it != warning_list.end()) {
        i = it - warning_list.begin();
        warning_list_frame->SetCursorIndex(i);
      }
    }

    if (i < 0)
      /* the selection may have changed, update CursorAirspace */
      AirspaceWarningCursorCallback(warning_list_frame->GetCursorIndex());
  } else {
    warning_list_frame->SetLength(1);
    selected_airspace = NULL;
  }
  warning_list_frame->Invalidate();
  UpdateButtons();
  AutoHide();
}

static void
OnTimer(gcc_unused WndForm &Sender)
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

  assert(warning_list.empty());

  airspace_warnings = &_warnings;

  dialog = LoadDialog(CallBackTable, parent, _T("IDR_XML_AIRSPACEWARNINGS"));
  assert(dialog != NULL);

  ack_warn_button = (WndButton *)dialog->FindByName(_T("frmAck1"));
  ack_day_button = (WndButton *)dialog->FindByName(_T("frmAck2"));
  ack_space_button = (WndButton *)dialog->FindByName(_T("frmAck"));
  enable_button = (WndButton *)dialog->FindByName(_T("frmEnable"));
  assert(ack_warn_button != NULL);
  assert(ack_day_button != NULL);
  assert(ack_space_button != NULL);
  assert(enable_button != NULL);

  dialog->SetKeyDownNotify(OnKeyDown);

  warning_list_frame =
    (ListControl*)dialog->FindByName(_T("frmAirspaceWarningList"));
  assert(warning_list_frame != NULL);
  warning_list_frame->SetPaintItemCallback(OnAirspaceListItemPaint);
  warning_list_frame->SetCursorCallback(AirspaceWarningCursorCallback);
  warning_list_frame->SetActivateCallback(OnAirspaceListEnter);

  auto_close = _auto_close;
  UpdateList();

  // JMW need to deselect everything on new reopening of dialog
  selected_airspace = NULL;
  focused_airspace = NULL;

  dialog->SetTimerNotify(OnTimer);
  warning_list_frame->SetCursorIndex(0);
  dialog->ShowModal();
  dialog->SetTimerNotify(NULL);

  delete dialog;

  // Needed for dlgAirspaceWarningVisible()
  dialog = NULL;

  warning_list.clear();
}
