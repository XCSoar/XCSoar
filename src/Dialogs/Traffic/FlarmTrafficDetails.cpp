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

/**
 * @file
 * The FLARM Traffic Details dialog displaying extended information about
 * the FLARM targets from the FLARMnet database
 * @todo Button that opens the Waypoint details dialog of the
 * home airport (if found in FLARMnet and local waypoint database)
 */

#include "TrafficDialogs.hpp"
#include "Look/TrafficLook.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "FLARM/FlarmNetRecord.hpp"
#include "FLARM/Traffic.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "FLARM/Friends.hpp"
#include "FLARM/Glue.hpp"
#include "Renderer/ColorButtonRenderer.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Util/StringBuilder.hxx"
#include "Util/StringCompare.hxx"
#include "Util/Macros.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Blackboard/LiveBlackboard.hpp"
#include "Blackboard/BlackboardListener.hpp"
#include "TeamActions.hpp"

class FlarmTrafficDetailsWidget final
  : public RowFormWidget, ActionListener, NullBlackboardListener {
  enum Controls {
    CALLSIGN,
    CHANGE_CALLSIGN_BUTTON,
    SPACER1,
    DISTANCE,
    ALTITUDE,
    VARIO,
    SPACER2,
    PILOT,
    AIRPORT,
    RADIO,
    PLANE,
  };

  enum Buttons {
    CHANGE_CALLSIGN,
    TEAM,
    CLEAR,
    GREEN,
    BLUE,
    YELLOW,
    MAGENTA,
  };

  WndForm &dialog;

  const FlarmId target_id;

public:
  FlarmTrafficDetailsWidget(WndForm &_dialog, FlarmId _target_id)
    :RowFormWidget(_dialog.GetLook()), dialog(_dialog),
     target_id(_target_id) {}

  void CreateButtons(WidgetDialog &buttons);

  /* virtual methods from Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Show(const PixelRect &rc) override;
  void Hide() override;

private:
  void UpdateChanging(const MoreData &basic);
  void Update();

  void OnCallsignClicked();
  void OnTeamClicked();
  void OnFriendColorClicked(FlarmColor color);

  /* virtual methods from ActionListener */
  void OnAction(int id) override;

  /* virtual methods from BlackboardListener */
  void OnGPSUpdate(const MoreData &basic) override {
    UpdateChanging(basic);
  }
};

inline void
FlarmTrafficDetailsWidget::CreateButtons(WidgetDialog &buttons)
{
  const ButtonLook &button_look = buttons.GetButtonLook();

  buttons.AddButton(new ColorButtonRenderer(button_look,
                                            TrafficLook::team_color_green),
                    *this, GREEN);

  buttons.AddButton(new ColorButtonRenderer(button_look,
                                            TrafficLook::team_color_blue),
                    *this, BLUE);

  buttons.AddButton(new ColorButtonRenderer(button_look,
                                            TrafficLook::team_color_yellow),
                    *this, YELLOW);

  buttons.AddButton(new ColorButtonRenderer(button_look,
                                            TrafficLook::team_color_magenta),
                    *this, MAGENTA);

  buttons.AddButton(_("Clear"), *this, CLEAR);
  buttons.AddButton(_("Team"), *this, TEAM);
}

void
FlarmTrafficDetailsWidget::Prepare(ContainerWindow &parent,
                                   const PixelRect &rc)
{
  AddReadOnly(_("Callsign"));
  AddButton(_("Change callsign"), *this, CHANGE_CALLSIGN);
  AddSpacer();
  AddReadOnly(_("Distance"));
  AddReadOnly(_("Altitude"));
  AddReadOnly(_("Vario"));
  AddSpacer();
  AddReadOnly(_("Pilot"));
  AddReadOnly(_("Airport"));
  AddReadOnly(_("Radio frequency"));
  AddReadOnly(_("Plane"));

  Update();
}

void
FlarmTrafficDetailsWidget::Show(const PixelRect &rc)
{
  RowFormWidget::Show(rc);
  Update();
  CommonInterface::GetLiveBlackboard().AddListener(*this);
}

void
FlarmTrafficDetailsWidget::Hide()
{
  CommonInterface::GetLiveBlackboard().RemoveListener(*this);
  RowFormWidget::Hide();
}

/**
 * Updates all the dialogs fields, that are changing frequently.
 * e.g. climb speed, distance, height
 */
void
FlarmTrafficDetailsWidget::UpdateChanging(const MoreData &basic)
{
  TCHAR tmp[40];
  const TCHAR *value;

  const FlarmTraffic* target =
    basic.flarm.traffic.FindTraffic(target_id);

  bool target_ok = target && target->IsDefined();

  // Fill distance/direction field
  if (target_ok) {
    FormatUserDistanceSmart(target->distance, tmp, 20, 1000);
    TCHAR *p = tmp + _tcslen(tmp);
    *p++ = _T(' ');
    FormatAngleDelta(p, 20, target->Bearing() - basic.track);
    value = tmp;
  } else
    value = _T("--");

  SetText(DISTANCE, value);

  // Fill altitude field
  if (target_ok) {
    TCHAR *p = tmp;
    if (target->altitude_available) {
      FormatUserAltitude(target->altitude, p, 20);
      p += _tcslen(p);
      *p++ = _T(' ');
    }

    Angle dir = Angle::FromXY(target->distance, target->relative_altitude);
    FormatVerticalAngleDelta(p, 20, dir);

    value = tmp;
  } else
    value = _T("--");

  SetText(ALTITUDE, value);

  // Fill climb speed field
  if (target_ok && target->climb_rate_avg30s_available) {
    FormatUserVerticalSpeed(target->climb_rate_avg30s, tmp, 20);
    value = tmp;
  } else
    value = _T("--");

  SetText(VARIO, value);
}

/**
 * Updates all the dialogs fields.
 * Should be called on dialog opening as it closes the dialog when the
 * target does not exist.
 */
void
FlarmTrafficDetailsWidget::Update()
{
  TCHAR tmp[200], tmp_id[7];
  const TCHAR *value;

  // Set the dialog caption
  StringFormatUnsafe(tmp, _T("%s (%s)"),
                     _("FLARM Traffic Details"), target_id.Format(tmp_id));
  dialog.SetCaption(tmp);

  // Try to find the target in the FLARMnet database
  /// @todo: make this code a little more usable
  const FlarmNetRecord *record = FlarmDetails::LookupRecord(target_id);
  if (record) {
    // Fill the pilot name field
    SetText(PILOT, record->pilot);

    // Fill the frequency field
    if (!StringIsEmpty(record->frequency))
      value = UnsafeBuildString(tmp, record->frequency.c_str(), _T(" MHz"));
    else
      value = _T("--");
    SetText(RADIO, value);

    // Fill the home airfield field
    SetText(AIRPORT, record->airfield);

    // Fill the plane type field
    SetText(PLANE, record->plane_type);
  } else {
    // Fill the pilot name field
    SetText(PILOT, _T("--"));

    // Fill the frequency field
    SetText(RADIO, _T("--"));

    // Fill the home airfield field
    SetText(AIRPORT, _T("--"));

    // Fill the plane type field
    const FlarmTraffic* target =
      CommonInterface::Basic().flarm.traffic.FindTraffic(target_id);

    const TCHAR* actype;
    if (target == nullptr ||
        (actype = FlarmTraffic::GetTypeString(target->type)) == nullptr)
      actype = _T("--");

    SetText(PLANE, actype);
  }

  // Fill the callsign field (+ registration)
  // note: don't use target->Name here since it is not updated
  //       yet if it was changed
  const TCHAR* cs = FlarmDetails::LookupCallsign(target_id);
  if (cs != nullptr && cs[0] != 0) {
    StringBuilder<TCHAR> builder(tmp, ARRAY_SIZE(tmp));
    builder.Append(cs);
    if (record)
      builder.Append(_T(" ("), record->registration.c_str(), _T(")"));
    value = tmp;
  } else
    value = _T("--");
  SetText(CALLSIGN, value);

  // Update the frequently changing fields too
  UpdateChanging(CommonInterface::Basic());
}

/**
 * This event handler is called when the "Team" button is pressed
 */
inline void
FlarmTrafficDetailsWidget::OnTeamClicked()
{
  // Ask for confirmation
  if (ShowMessageBox(_("Do you want to set this FLARM contact as your new teammate?"),
                  _("New Teammate"), MB_YESNO) != IDYES)
    return;

  TeamActions::TrackFlarm(target_id);

  // Close the dialog
  dialog.SetModalResult(mrOK);
}

/**
 * This event handler is called when the "Change Callsign" button is pressed
 */
inline void
FlarmTrafficDetailsWidget::OnCallsignClicked()
{
  StaticString<21> newName;
  newName.clear();
  if (TextEntryDialog(newName, _("Competition ID")) &&
      FlarmDetails::AddSecondaryItem(target_id, newName))
    SaveFlarmNames();

  Update();
}

void
FlarmTrafficDetailsWidget::OnFriendColorClicked(FlarmColor color)
{
  FlarmFriends::SetFriendColor(target_id, color);
  dialog.SetModalResult(mrOK);
}

void
FlarmTrafficDetailsWidget::OnAction(int id)
{
  switch (id) {
  case CHANGE_CALLSIGN:
    OnCallsignClicked();
    break;

  case TEAM:
    OnTeamClicked();
    break;

  case CLEAR:
    OnFriendColorClicked(FlarmColor::NONE);
    break;

  case GREEN:
    OnFriendColorClicked(FlarmColor::GREEN);
    break;

  case BLUE:
    OnFriendColorClicked(FlarmColor::BLUE);
    break;

  case YELLOW:
    OnFriendColorClicked(FlarmColor::YELLOW);
    break;

  case MAGENTA:
    OnFriendColorClicked(FlarmColor::MAGENTA);
    break;
  }
}

/**
 * The function opens the FLARM Traffic Details dialog
 */
void
dlgFlarmTrafficDetailsShowModal(FlarmId id)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(look);

  FlarmTrafficDetailsWidget *widget =
    new FlarmTrafficDetailsWidget(dialog, id);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("FLARM Traffic Details"),
                    widget);
  widget->CreateButtons(dialog);
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
