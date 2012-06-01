/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Dialogs/Airspace.hpp"
#include "Dialogs/CallBackTable.hpp"
#include "Dialogs/Internal.hpp"
#include "Dialogs/Dialogs.h"
#include "Airspace/AirspaceSorter.hpp"
#include "Math/FastMath.h"
#include "Math/Earth.hpp"
#include "Form/DataField/String.hpp"
#include "Form/DataField/Enum.hpp"
#include "Engine/Airspace/Airspaces.hpp"
#include "Engine/Airspace/AbstractAirspace.hpp"
#include "Renderer/AirspaceListRenderer.hpp"
#include "Look/MapLook.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Busy.hpp"
#include "Compiler.h"
#include "Util/Macros.hpp"
#include "Units/Units.hpp"
#include "Formatter/AngleFormatter.hpp"
#include "UIGlobals.hpp"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * Special enum integer value for "filter disabled".
 */
static gcc_constexpr_data unsigned WILDCARD = 0x7fff;

static const Airspaces *airspaces;
static ProtectedAirspaceWarningManager *airspace_warnings;

static GeoPoint location;

static WndForm *dialog;
static WndProperty *name_control;
static WndProperty *distance_control;
static WndProperty *direction_control;
static ListControl *airspace_list_control;

static fixed distance_filter;

static unsigned direction_filter = WILDCARD;
static Angle last_heading;

static gcc_constexpr_data StaticEnumChoice type_filter_list[] = {
  { WILDCARD, _T("*") },
  { OTHER, _T("Other") },
  { RESTRICT, _T("Restricted areas") },
  { PROHIBITED, _T("Prohibited areas") },
  { DANGER, _T("Danger areas") },
  { CLASSA, _T("Class A") },
  { CLASSB, _T("Class B") },
  { CLASSC, _T("Class C") },
  { CLASSD, _T("Class D") },
  { NOGLIDER, _T("No gliders") },
  { CTR, _T("CTR") },
  { WAVE, _T("Wave") },
  { CLASSE, _T("Class E") },
  { CLASSF, _T("Class F") },
  { 0 }
};

static unsigned type_filter = WILDCARD;

static AirspaceSelectInfoVector airspace_list;

static AirspaceSorter* airspace_sorter;

static void
OnAirspaceListEnter(unsigned i)
{
  if (airspace_list.empty()) {
    assert(i == 0);
    return;
  }

  assert(i < airspace_list.size());

  dlgAirspaceDetails(*airspace_list[i].airspace, airspace_warnings);
}


static void
UpdateList()
{
  airspace_list = airspace_sorter->GetList();

  if (type_filter != WILDCARD)
    airspace_sorter->FilterByClass(airspace_list, (AirspaceClass)type_filter);
  
  bool sort_distance = false;
  if (positive(distance_filter)) {
    sort_distance = true;
    airspace_sorter->FilterByDistance(airspace_list, distance_filter);
  } 

  if (direction_filter != WILDCARD) {
    sort_distance = true;
    Angle a = direction_filter == 0
      ? CommonInterface::Calculated().heading
      : Angle::Degrees(fixed(direction_filter));
    airspace_sorter->FilterByDirection(airspace_list, a);
  }

  if (sort_distance)
    airspace_sorter->SortByDistance(airspace_list);


  const TCHAR *name_filter = name_control->GetDataField()->GetAsString();
  if (!StringIsEmpty(name_filter))
    airspace_sorter->FilterByNamePrefix(airspace_list, name_filter);

  airspace_list_control->SetLength(max((size_t)1, airspace_list.size()));
  airspace_list_control->Invalidate();
}

static void
FilterMode(bool direction)
{
  if (direction) {
    distance_filter = fixed_minus_one;
    direction_filter = WILDCARD;

    DataFieldEnum *df = (DataFieldEnum *)distance_control->GetDataField();
    df->Set(WILDCARD);
    distance_control->RefreshDisplay();

    df = (DataFieldEnum *)direction_control->GetDataField();
    df->Set(WILDCARD);
    direction_control->RefreshDisplay();
  } else {
    DataFieldString *df = (DataFieldString *)name_control->GetDataField();
    df->Set(_T(""));
    name_control->RefreshDisplay();
  }
}

static void
OnFilterName(DataField *_Sender, DataField::DataAccessMode Mode)
{
  switch (Mode) {
  case DataField::daChange:
    FilterMode(true);
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnFilterDistance(DataField *_Sender, DataField::DataAccessMode Mode)
{
  DataFieldString *Sender = (DataFieldString *)_Sender;

  switch (Mode) {
  case DataField::daChange:
    distance_filter = (unsigned)Sender->GetAsInteger() != WILDCARD
      ? Units::ToSysDistance(fixed(Sender->GetAsInteger()))
      : fixed_minus_one;
    FilterMode(false);
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnFilterDirection(DataField *_Sender, DataField::DataAccessMode Mode)
{
  DataFieldEnum &df = *(DataFieldEnum *)_Sender;

  switch (Mode) {
  case DataField::daChange:
    direction_filter = df.GetAsInteger();
    FilterMode(false);
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnFilterType(DataField *Sender, DataField::DataAccessMode Mode)
{
  switch (Mode) {
  case DataField::daChange:
    type_filter = Sender->GetAsInteger();
    FilterMode(false);
    UpdateList();
    break;

  case DataField::daSpecial:
    return;
  }
}

static void
OnPaintListItem(Canvas &canvas, const PixelRect rc, unsigned i)
{
  if (airspace_list.empty()) {
    assert(i == 0);

    canvas.text(rc.left + Layout::FastScale(2),
                rc.top + Layout::FastScale(2), _("No Match!"));
    return;
  }

  assert(i < airspace_list.size());

  const AbstractAirspace &airspace = *airspace_list[i].airspace;

  AirspaceListRenderer::Draw(
      canvas, rc, airspace,
      airspace_list[i].GetVector(location, airspaces->GetProjection()),
      UIGlobals::GetDialogLook(), UIGlobals::GetMapLook().airspace,
      CommonInterface::GetMapSettings().airspace);
}


static void
CloseClicked(gcc_unused WndButton &button)
{
  dialog->SetModalResult(mrCancel);
}

gcc_pure
static const TCHAR *
GetHeadingString(TCHAR *buffer)
{
  TCHAR heading[32];
  FormatBearing(heading, ARRAY_SIZE(heading),
                CommonInterface::Calculated().heading);

  _stprintf(buffer, _T("%s (%s)"), _("Heading"), heading);
  return buffer;
}

static void
OnTimerNotify(gcc_unused WndForm &Sender)
{
  if (direction_filter == 0 && !CommonInterface::Calculated().circling) {
    Angle a = last_heading - CommonInterface::Calculated().heading;
    if (a.AsDelta().AbsoluteDegrees() >= fixed(10)) {
      last_heading = CommonInterface::Calculated().heading;

      UpdateList();

      DataFieldEnum &df = *(DataFieldEnum *)direction_control->GetDataField();
      TCHAR buffer[64];
      df.replaceEnumText(0, GetHeadingString(buffer));
      direction_control->RefreshDisplay();
    }
  }
}

#ifdef GNAV

static bool
FormKeyDown(WndForm &Sender, unsigned key_code)
{
  WndProperty* wp;
  unsigned new_index = type_filter;

  wp = ((WndProperty *)dialog->FindByName(_T("prpFltType")));

  switch(key_code) {
  case VK_APP1:
      NewIndex = WILDCARD;
    break;
  case VK_APP2:
      NewIndex = RESTRICT;
    break;
  case VK_APP3:
      NewIndex = PROHIBITED;
    break;

  default:
    return false;
  }

  if (type_filter != new_index){
    wp->GetDataField()->SetAsInteger(NewIndex);
    wp->RefreshDisplay();
  }

  return true;
}

#endif /* GNAV */

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnFilterName),
  DeclareCallBackEntry(OnFilterDistance),
  DeclareCallBackEntry(OnFilterDirection),
  DeclareCallBackEntry(OnFilterType),
  DeclareCallBackEntry(CloseClicked),
  DeclareCallBackEntry(NULL)
};

static void
FillDistanceEnum(DataFieldEnum &df)
{
  df.AddChoice(0, _T("*"));

  static gcc_constexpr_data unsigned distances[] = {
    25, 50, 75, 100, 150, 250, 500, 1000
  };

  TCHAR buffer[64];
  const TCHAR *unit = Units::GetDistanceName();
  for (unsigned i = 0; i < ARRAY_SIZE(distances); ++i) {
    _stprintf(buffer, _T("%u %s"), distances[i], unit);
    df.AddChoice(distances[i], buffer);
  }

  df.Set(0);
}

static void
FillDirectionEnum(DataFieldEnum &df)
{
  TCHAR buffer[64];

  df.AddChoice(WILDCARD, _T("*"));
  df.AddChoice(0, GetHeadingString(buffer));

  static gcc_constexpr_data unsigned directions[] = {
    360, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330
  };

  for (unsigned i = 0; i < ARRAY_SIZE(directions); ++i) {
    FormatBearing(buffer, ARRAY_SIZE(buffer), directions[i]);
    df.AddChoice(directions[i], buffer);
  }

  df.Set(WILDCARD);
}

static void
PrepareAirspaceSelectDialog()
{
  gcc_unused ScopeBusyIndicator busy;

  dialog = LoadDialog(CallBackTable, UIGlobals::GetMainWindow(),
                  Layout::landscape ? _T("IDR_XML_AIRSPACESELECT_L") :
                                      _T("IDR_XML_AIRSPACESELECT"));
  assert(dialog != NULL);

#ifdef GNAV
  dialog->SetKeyDownNotify(FormKeyDown);
#endif

  const DialogLook &dialog_look = UIGlobals::GetDialogLook();

  airspace_list_control = (ListControl*)dialog->FindByName(_T("frmAirspaceList"));
  assert(airspace_list_control != NULL);
  airspace_list_control->SetActivateCallback(OnAirspaceListEnter);
  airspace_list_control->SetPaintItemCallback(OnPaintListItem);
  airspace_list_control->SetItemHeight(AirspaceListRenderer::GetHeight(dialog_look));

  name_control = (WndProperty*)dialog->FindByName(_T("prpFltName"));
  assert(name_control != NULL);

  distance_control = (WndProperty*)dialog->FindByName(_T("prpFltDistance"));
  assert(distance_control != NULL);
  FillDistanceEnum(*(DataFieldEnum *)distance_control->GetDataField());
  distance_control->RefreshDisplay();

  direction_control = (WndProperty*)dialog->FindByName(_T("prpFltDirection"));
  assert(direction_control != NULL);
  FillDirectionEnum(*(DataFieldEnum *)direction_control->GetDataField());
  direction_control->RefreshDisplay();

  LoadFormProperty(*dialog, _T("prpFltType"), type_filter_list, WILDCARD);

  dialog->SetTimerNotify(OnTimerNotify);
}

void
ShowAirspaceListDialog(const Airspaces &_airspaces,
                       ProtectedAirspaceWarningManager *_airspace_warnings)
{
  airspace_warnings = _airspace_warnings;
  airspaces = &_airspaces;
  location = XCSoarInterface::Basic().location;

  PrepareAirspaceSelectDialog();

  AirspaceSorter _airspace_sorter(*airspaces, location);
  airspace_sorter = &_airspace_sorter;

  UpdateList();

  dialog->ShowModal();
  delete dialog;
}

