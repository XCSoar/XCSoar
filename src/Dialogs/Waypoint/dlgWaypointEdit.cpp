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

#include "WaypointDialogs.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Form.hpp"
#include "Form/Util.hpp"
#include "Form/Button.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/GeoPoint.hpp"
#include "Units/Units.hpp"
#include "Screen/Layout.hpp"
#include "Terrain/RasterTerrain.hpp"
#include "UIGlobals.hpp"
#include "Components.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Util/StringUtil.hpp"
#include "Compiler.h"
#include "Sizes.h"
#include "Interface.hpp"
#include "Language/Language.hpp"

#include <stdio.h>

static WndForm *wf = NULL;
static Waypoint *global_wpt = NULL;

static void
SetValues()
{
  LoadFormProperty(*wf, _T("Name"), global_wpt->name.c_str());
  LoadFormProperty(*wf, _T("Comment"), global_wpt->comment.c_str());

  WndProperty *wp = (WndProperty *)wf->FindByName(_T("Location"));
  assert(wp != NULL);

  GeoPointDataField &gpdf = *(GeoPointDataField *)wp->GetDataField();
  gpdf.SetValue(global_wpt->location);
  wp->RefreshDisplay();

  LoadFormProperty(*wf, _T("prpAltitude"), UnitGroup::ALTITUDE, global_wpt->elevation);

  wp = (WndProperty*)wf->FindByName(_T("prpFlags"));
  assert(wp != NULL);
  DataFieldEnum *dfe = (DataFieldEnum*)wp->GetDataField();

  dfe->addEnumText(_T("Turnpoint"));
  dfe->addEnumText(_T("Airport"));
  dfe->addEnumText(_T("Landpoint"));

  if (global_wpt->IsAirport())
    dfe->Set(1u);
  else if (global_wpt->IsLandable())
    dfe->Set(2u);
  else
    dfe->Set(0u);

  wp->RefreshDisplay();
}

static void
GetValues()
{
  global_wpt->name = GetFormValueString(*wf, _T("Name"));
  global_wpt->comment = GetFormValueString(*wf, _T("Comment"));

  WndProperty *wp = (WndProperty *)wf->FindByName(_T("Location"));
  assert(wp != NULL);

  const GeoPointDataField &gpdf =
    *(const GeoPointDataField *)wp->GetDataField();
  global_wpt->location = gpdf.GetValue();

  int ss = GetFormValueInteger(*wf, _T("prpAltitude"));
  global_wpt->elevation = (ss == 0 && terrain != NULL)
    ? fixed(terrain->GetTerrainHeight(global_wpt->location))
    : Units::ToSysAltitude(fixed(ss));

  wp = (WndProperty*)wf->FindByName(_T("prpFlags"));
  assert(wp != NULL);
  switch (((const DataFieldEnum *)wp->GetDataField())->GetValue()) {
  case 1:
    global_wpt->flags.turn_point = true;
    global_wpt->type = Waypoint::Type::AIRFIELD;
    break;
  case 2:
    global_wpt->type = Waypoint::Type::OUTLANDING;
    break;
  default:
    global_wpt->type = Waypoint::Type::NORMAL;
    global_wpt->flags.turn_point = true;
  };
}

bool
dlgWaypointEditShowModal(Waypoint &way_point)
{
  global_wpt = &way_point;

  wf = LoadDialog(nullptr, UIGlobals::GetMainWindow(),
                  Layout::landscape ?
                  _T("IDR_XML_WAYPOINTEDIT_L") : _T("IDR_XML_WAYPOINTEDIT"));
  assert(wf != NULL);

  SetValues();

  if (CommonInterface::GetUISettings().coordinate_format ==
      CoordinateFormat::UTM) {
    ShowMessageBox(
        _("Sorry, the waypoint editor is not yet available for the UTM coordinate format."),
        _("Waypoint Editor"), MB_OK);
    return false;
  }

  wf->SetModalResult(mrCancel);

  bool retval = false;
  if (wf->ShowModal() == mrOK) {
    GetValues();
    retval = true;
  }

  delete wf;
  return retval;
}
