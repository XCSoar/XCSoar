/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Dialogs/MapItemListDialog.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Fonts.hpp"
#include "Dialogs/Airspace.hpp"
#include "Dialogs/Task.hpp"
#include "Dialogs/Waypoint.hpp"
#include "Language/Language.hpp"
#include "Engine/Navigation/Geometry/GeoVector.hpp"
#include "MapWindow/MapItem.hpp"
#include "MapWindow/MapItemList.hpp"
#include "MapWindow/MapItemListBuilder.hpp"
#include "Renderer/MapItemListRenderer.hpp"
#include "Dialogs/XML.hpp"
#include "Dialogs/dlgTools.h"
#include "Form/Form.hpp"
#include "Form/Frame.hpp"
#include "Form/List.hpp"
#include "Form/Button.hpp"
#include "Units/Units.hpp"
#include "Units/UnitsFormatter.hpp"

static const AircraftLook *aircraft_look;
static const AirspaceLook *airspace_look;
static const WaypointLook *waypoint_look;
static const TaskLook *task_look;
static const MarkerLook *marker_look;
static const SETTINGS_MAP *settings;
static GeoVector vector;
static const MapItemList *list;
static WndForm *wf;
static WndButton *details_button;

static bool
HasDetails(const MapItem &item)
{
  switch (item.type) {
  case MapItem::SELF:
  case MapItem::MARKER:
    return false;

  case MapItem::AIRSPACE:
  case MapItem::WAYPOINT:
  case MapItem::TASK_OZ:
    return true;
  }

  return false;
}

static void
PaintListItem(Canvas &canvas, const PixelRect rc, unsigned idx)
{
  const MapItem &item = *(*list)[idx];
  MapItemListRenderer::Draw(canvas, rc, item, *aircraft_look, *airspace_look,
                            *waypoint_look, *task_look, *marker_look, *settings);
}

static void
OnListIndexChange(unsigned i)
{
  details_button->set_enabled(HasDetails(*(*list)[i]));
}

static void
OnDetailsClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrOK);
}

static void
OnComboPopupListEnter(gcc_unused unsigned i)
{
  wf->SetModalResult(mrOK);
}

static void
OnCancelClicked(gcc_unused WndButton &Sender)
{
  wf->SetModalResult(mrCancel);
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnDetailsClicked),
  DeclareCallBackEntry(OnCancelClicked),
  DeclareCallBackEntry(NULL)
};

static int
ShowMapItemListDialog(SingleWindow &parent)
{
  unsigned num_items = list->size();

  UPixelScalar item_height = Fonts::MapBold.GetHeight() + Layout::Scale(6) +
                             Fonts::MapLabel.GetHeight();

  assert(num_items <= 0x7fffffff);
  assert(item_height > 0);

  wf = LoadDialog(CallBackTable, parent, Layout::landscape ?
                  _T("IDR_XML_MAPITEMLIST_L") : _T("IDR_XML_MAPITEMLIST"));
  assert(wf != NULL);

  details_button = (WndButton *)wf->FindByName(_T("cmdDetails"));
  assert(details_button);

  WndListFrame *list_control =
      (WndListFrame *)wf->FindByName(_T("frmComboPopupList"));
  assert(list_control != NULL);
  list_control->SetItemHeight(item_height);
  list_control->SetLength(num_items);
  list_control->SetCursorIndex(0);
  list_control->SetActivateCallback(OnComboPopupListEnter);
  list_control->SetPaintItemCallback(PaintListItem);
  list_control->SetCursorCallback(OnListIndexChange);
  OnListIndexChange(0);

  TCHAR info_buffer[256], distance_buffer[32], direction_buffer[32];
  if (vector.IsValid()) {
    Units::FormatUserDistance(vector.distance, distance_buffer, 32);
    _stprintf(direction_buffer, _T("%.0f " DEG "T"),
              (double)vector.bearing.Degrees());
    _stprintf(info_buffer, _T("%s: %s - %s: %s"),
              _("Distance"), distance_buffer,
              _("Direction"), direction_buffer);
  } else {
    _stprintf(info_buffer, _T("%s: %s - %s: %s"),
              _("Distance"), _T("???"), _("Direction"), _T("???"));
  }
  WndFrame *info_label = (WndFrame *)wf->FindByName(_T("lblInfo"));
  assert(info_label);
  info_label->SetAlignCenter();
  info_label->SetVAlignCenter();
  info_label->SetCaption(info_buffer);

  int result = wf->ShowModal() == mrOK
    ? (int)list_control->GetCursorIndex()
    : -1;
  delete wf;

  return result;
}

void
ShowMapItemDialog(const MapItem &item, SingleWindow &parent)
{
  switch (item.type) {
  case MapItem::SELF:
  case MapItem::MARKER:
    break;

  case MapItem::AIRSPACE:
    dlgAirspaceDetails(*((const AirspaceMapItem &)item).airspace);
    break;
  case MapItem::WAYPOINT:
    dlgWaypointDetailsShowModal(parent,
                                ((const WaypointMapItem &)item).waypoint);
    break;
  case MapItem::TASK_OZ:
    dlgTargetShowModal(((const TaskOZMapItem &)item).index);
    break;
  }
}

void
ShowMapItemListDialog(SingleWindow &parent,
                      const GeoVector &_vector,
                      const MapItemList &_list,
                      const AircraftLook &_aircraft_look,
                      const AirspaceLook &_airspace_look,
                      const WaypointLook &_waypoint_look,
                      const TaskLook &_task_look,
                      const MarkerLook &_marker_look,
                      const SETTINGS_MAP &_settings)
{
  switch (_list.size()) {
  case 0:
    /* no map items in the list */
    return;

  case 1:
    /* only one map item, show it */
    ShowMapItemDialog(*_list[0], parent);
    break;

  default:
    /* more than one map item: show a list */
    vector = _vector;
    list = &_list;

    aircraft_look = &_aircraft_look;
    airspace_look = &_airspace_look;
    waypoint_look = &_waypoint_look;
    task_look = &_task_look;
    marker_look = &_marker_look;
    settings = &_settings;

    int i = ShowMapItemListDialog(parent);
    assert(i >= -1 && i < (int)_list.size());
    if (i >= 0)
      ShowMapItemDialog(*_list[i], parent);
  }
}
