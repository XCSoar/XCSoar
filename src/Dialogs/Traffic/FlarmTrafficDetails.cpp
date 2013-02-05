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

/**
 * @file
 * The FLARM Traffic Details dialog displaying extended information about
 * the FLARM targets from the FLARMnet database
 * @todo Button that opens the Waypoint details dialog of the
 * home airport (if found in FLARMnet and local waypoint database)
 */

#include "TrafficDialogs.hpp"
#include "Dialogs/TextEntry.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "FLARM/FlarmNet.hpp"
#include "FLARM/FlarmNetRecord.hpp"
#include "FLARM/Traffic.hpp"
#include "FLARM/FlarmDetails.hpp"
#include "FLARM/Friends.hpp"
#include "FLARM/Glue.hpp"
#include "Screen/Layout.hpp"
#include "Geo/Math.hpp"
#include "LocalPath.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "Formatter/UserUnits.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "Util/StringUtil.hpp"
#include "Util/Macros.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"
#include "Blackboard/ScopeGPSListener.hpp"
#include "TeamActions.hpp"
#include "Compiler.h"

#include <math.h>
#include <stdio.h>

static WndForm *wf = NULL;
static FlarmId target_id;

/**
 * Updates all the dialogs fields, that are changing frequently.
 * e.g. climb speed, distance, height
 */
static void
UpdateChanging(const MoreData &basic)
{
  TCHAR tmp[20];
  const FlarmTraffic* target =
    basic.flarm.traffic.FindTraffic(target_id);

  bool target_ok = target && target->IsDefined();

  // Fill distance field
  if (target_ok)
    FormatUserDistanceSmart(target->distance, tmp, 20, fixed(1000));
  else
    _tcscpy(tmp, _T("--"));
  SetFormValue(*wf, _T("prpDistance"), tmp);

  // Fill horizontal direction field
  if (target_ok)
    FormatAngleDelta(tmp, ARRAY_SIZE(tmp),
                     target->Bearing() - basic.track);
  else
    _tcscpy(tmp, _T("--"));
  SetFormValue(*wf, _T("prpDirectionH"), tmp);

  // Fill altitude field
  if (target_ok && target->altitude_available)
    FormatUserAltitude(target->altitude, tmp, 20);
  else
    _tcscpy(tmp, _T("--"));
  SetFormValue(*wf, _T("prpAltitude"), tmp);

  // Fill vertical direction field
  if (target_ok) {
    Angle dir = Angle::FromXY(target->distance, target->relative_altitude);
    FormatVerticalAngleDelta(tmp, ARRAY_SIZE(tmp), dir);
  } else
    _tcscpy(tmp, _T("--"));
  SetFormValue(*wf, _T("prpDirectionV"), tmp);

  // Fill climb speed field
  if (target_ok && target->climb_rate_avg30s_available)
    FormatUserVerticalSpeed(target->climb_rate_avg30s, tmp, 20);
  else
    _tcscpy(tmp, _T("--"));
  SetFormValue(*wf, _T("prpVSpeed"), tmp);
}

/**
 * Updates all the dialogs fields.
 * Should be called on dialog opening as it closes the dialog when the
 * target does not exist.
 */
static void
Update()
{
  TCHAR tmp[200], tmp_id[7];

  // Set the dialog caption
  _stprintf(tmp, _T("%s (%s)"),
            _("FLARM Traffic Details"), target_id.Format(tmp_id));
  wf->SetCaption(tmp);

  // Try to find the target in the FLARMnet database
  /// @todo: make this code a little more usable
  const FlarmNetRecord *record = FlarmDetails::LookupRecord(target_id);
  if (record) {
    // Fill the pilot name field
    SetFormValue(*wf, _T("prpPilot"), record->pilot);

    // Fill the frequency field
    if (!StringIsEmpty(record->frequency)) {
      _tcscpy(tmp, record->frequency);
      _tcscat(tmp, _T(" MHz"));
      SetFormValue(*wf, _T("prpFrequency"), tmp);
    } else
      SetFormValue(*wf, _T("prpFrequency"), _T("--"));

    // Fill the home airfield field
    SetFormValue(*wf, _T("prpAirport"), record->airfield);

    // Fill the plane type field
    SetFormValue(*wf, _T("prpPlaneType"), record->plane_type);
  } else {
    // Fill the pilot name field
    SetFormValue(*wf, _T("prpPilot"), _T("--"));

    // Fill the frequency field
    SetFormValue(*wf, _T("prpFrequency"), _T("--"));

    // Fill the home airfield field
    SetFormValue(*wf, _T("prpAirport"), _T("--"));

    // Fill the plane type field
    const FlarmTraffic* target =
      CommonInterface::Basic().flarm.traffic.FindTraffic(target_id);

    const TCHAR* actype;
    if (target == NULL ||
        (actype = FlarmTraffic::GetTypeString(target->type)) == NULL)
      actype = _T("--");

    SetFormValue(*wf, _T("prpPlaneType"), actype);
  }

  // Fill the callsign field (+ registration)
  // note: don't use target->Name here since it is not updated
  //       yet if it was changed
  const TCHAR* cs = FlarmDetails::LookupCallsign(target_id);
  if (cs != NULL && cs[0] != 0) {
    _tcscpy(tmp, cs);
    if (record) {
      _tcscat(tmp, _T(" ("));
      _tcscat(tmp, record->registration);
      _tcscat(tmp, _T(")"));
    }
  } else
    _tcscpy(tmp, _T("--"));
  SetFormValue(*wf, _T("prpCallsign"), tmp);

  // Update the frequently changing fields too
  UpdateChanging(CommonInterface::Basic());
}

/**
 * This event handler is called when the "Team" button is pressed
 */
static void
OnTeamClicked()
{
  // Ask for confirmation
  if (ShowMessageBox(_("Do you want to set this FLARM contact as your new teammate?"),
                  _("New Teammate"), MB_YESNO) != IDYES)
    return;

  TeamActions::TrackFlarm(target_id);

  // Close the dialog
  wf->SetModalResult(mrOK);
}

/**
 * This event handler is called when the "Change Callsign" button is pressed
 */
static void
OnCallsignClicked()
{
  StaticString<21> newName;
  newName.clear();
  if (TextEntryDialog(newName, _("Competition ID")) &&
      FlarmDetails::AddSecondaryItem(target_id, newName))
    SaveFlarmNames();

  Update();
}

static void
OnFriendBlueClicked()
{
  FlarmFriends::SetFriendColor(target_id, FlarmColor::BLUE);
  wf->SetModalResult(mrOK);
}

static void
OnFriendGreenClicked()
{
  FlarmFriends::SetFriendColor(target_id, FlarmColor::GREEN);
  wf->SetModalResult(mrOK);
}

static void
OnFriendYellowClicked()
{
  FlarmFriends::SetFriendColor(target_id, FlarmColor::YELLOW);
  wf->SetModalResult(mrOK);
}

static void
OnFriendMagentaClicked()
{
  FlarmFriends::SetFriendColor(target_id, FlarmColor::MAGENTA);
  wf->SetModalResult(mrOK);
}

static void
OnFriendClearClicked()
{
  FlarmFriends::SetFriendColor(target_id, FlarmColor::NONE);
  wf->SetModalResult(mrOK);
}

static constexpr CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnTeamClicked),
  DeclareCallBackEntry(OnCallsignClicked),
  DeclareCallBackEntry(OnFriendGreenClicked),
  DeclareCallBackEntry(OnFriendBlueClicked),
  DeclareCallBackEntry(OnFriendYellowClicked),
  DeclareCallBackEntry(OnFriendMagentaClicked),
  DeclareCallBackEntry(OnFriendClearClicked),
  DeclareCallBackEntry(NULL)
};

/**
 * The function opens the FLARM Traffic Details dialog
 */
void
dlgFlarmTrafficDetailsShowModal(FlarmId id)
{
  if (wf)
    return;

  target_id = id;

  // Load dialog from XML
  wf = LoadDialog(CallBackTable,
      UIGlobals::GetMainWindow(), Layout::landscape ?
      _T("IDR_XML_FLARMTRAFFICDETAILS_L") : _T("IDR_XML_FLARMTRAFFICDETAILS"));
  assert(wf != NULL);

  // Update fields for the first time
  Update();

  const ScopeGPSListener l(CommonInterface::GetLiveBlackboard(), UpdateChanging);

  // Show the dialog
  wf->ShowModal();

  // After dialog closed -> delete it
  delete wf;
  wf = NULL;
}
