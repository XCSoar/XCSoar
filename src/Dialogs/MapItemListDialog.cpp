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

#include "Dialogs/MapItemListDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Screen/Canvas.hpp"
#include "Dialogs/Airspace/Airspace.hpp"
#include "Dialogs/Task/TaskDialogs.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Traffic/TrafficDialogs.hpp"
#include "Dialogs/Weather/WeatherDialog.hpp"
#include "Language/Language.hpp"
#include "MapSettings.hpp"
#include "MapWindow/Items/MapItem.hpp"
#include "MapWindow/Items/List.hpp"
#include "Renderer/MapItemListRenderer.hpp"
#include "Widget/ListWidget.hpp"
#include "Form/Button.hpp"
#include "Weather/Features.hpp"
#include "Components.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Airspace/ProtectedAirspaceWarningManager.hpp"
#include "Interface.hpp"
#include "UIGlobals.hpp"
#include "Profile/Profile.hpp"

#ifdef HAVE_NOAA
#include "Dialogs/Weather/NOAADetails.hpp"
#endif

static bool
HasDetails(const MapItem &item)
{
  switch (item.type) {
  case MapItem::LOCATION:
  case MapItem::ARRIVAL_ALTITUDE:
  case MapItem::SELF:
  case MapItem::THERMAL:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::SKYLINES_TRAFFIC:
#endif
    return false;

  case MapItem::AIRSPACE:
  case MapItem::WAYPOINT:
  case MapItem::TASK_OZ:
  case MapItem::TRAFFIC:
#ifdef HAVE_NOAA
  case MapItem::WEATHER:
#endif
  case MapItem::OVERLAY:
  case MapItem::RASP:
    return true;
  }

  return false;
}

class MapItemListWidget final
: public ListWidget, private ActionListener {
  enum Buttons {
    SETTINGS,
    GOTO,
    ACK,
  };

  const MapItemList &list;

  const DialogLook &dialog_look;
  const MapSettings &settings;

  MapItemListRenderer renderer;

  Button *settings_button, *details_button, *cancel_button, *goto_button;
  Button *ack_button;

public:
  void
  CreateButtons(WidgetDialog &dialog);

  /*
   * bsetting_done == true the MapItemList settings changed, a refresh of the MapItemList is necessary
   */
  bool bsetting_done = false;

public:
  MapItemListWidget(const MapItemList &_list, const DialogLook &_dialog_look,
                    const MapLook &_look, const TrafficLook &_traffic_look,
                    const FinalGlideBarLook &_final_glide_look,
                    const MapSettings &_settings)
      : list(_list), dialog_look(_dialog_look), settings(_settings), renderer(
          _look, _traffic_look, _final_glide_look, _settings,
          CommonInterface::GetComputerSettings().utc_offset)
  {
  }

  unsigned
  GetCursorIndex() const
  {
    return GetList().GetCursorIndex();
  }

protected:
  void
  UpdateButtons()
  {
    const unsigned current = GetCursorIndex();
    details_button->SetEnabled(HasDetails(*list[current]));
    goto_button->SetEnabled(CanGotoItem(current));
    ack_button->SetEnabled(CanAckItem(current));
  }

  void
  OnGotoClicked();
  void
  OnAckClicked();

public:
  /* virtual methods from class Widget */
  virtual void
  Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  virtual void
  Unprepare() override
  {
    DeleteWindow();
  }

  /* virtual methods from class List::Handler */
  virtual void
  OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx) override;

  virtual void
  OnCursorMoved(unsigned index) override
  {
    UpdateButtons();
  }

  virtual bool
  CanActivateItem(unsigned index) const override
  {
    return HasDetails(*list[index]);
  }

  bool
  CanGotoItem(unsigned index) const
  {
    return CanGotoItem(*list[index]);
  }

  static bool
  CanGotoItem(const MapItem &item)
  {
    return protected_task_manager != NULL && item.type == MapItem::WAYPOINT;
  }

  bool
  CanAckItem(unsigned index) const
  {
    return CanAckItem(*list[index]);
  }

  static bool
  CanAckItem(const MapItem &item)
  {
    const AirspaceMapItem &as_item = (const AirspaceMapItem &)item;

    return item.type == MapItem::AIRSPACE && GetAirspaceWarnings() != nullptr
        && !GetAirspaceWarnings()->GetAckDay(*as_item.airspace);
  }

  virtual void
  OnActivateItem(unsigned index) override;

  /* virtual methods from class ActionListener */
  virtual void
  OnAction(int id) override;
};

void
MapItemListWidget::CreateButtons(WidgetDialog &dialog)
{
  settings_button = dialog.AddButton(_("Settings"), *this, SETTINGS);
  goto_button = dialog.AddButton(_("Goto"), *this, GOTO);
  ack_button = dialog.AddButton(_("Ack Day"), *this, ACK);
  details_button = dialog.AddButton(_("Details"), mrOK);
  cancel_button = dialog.AddButton(_("Close"), mrCancel);
}

void
MapItemListWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  CreateList(parent, dialog_look, rc, renderer.CalculateLayout(dialog_look));

  GetList().SetLength(list.size());
  UpdateButtons();

  for (unsigned i = 0; i < list.size(); ++i) {
    const MapItem &item = *list[i];
    if (HasDetails(item) || CanGotoItem(item)) {
      GetList().SetCursorIndex(i);
      break;
    }
  }
}

void
MapItemListWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                               unsigned idx)
{
  const MapItem &item = *list[idx];
  renderer.Draw(canvas, rc, item, &CommonInterface::Basic().flarm.traffic);

  if ((settings.item_list.add_arrival_altitude
      && item.type == MapItem::Type::ARRIVAL_ALTITUDE)
      || (!settings.item_list.add_arrival_altitude
          && item.type == MapItem::Type::LOCATION)) {
    canvas.SelectBlackPen();
    canvas.DrawLine(rc.left, rc.bottom - 1, rc.right, rc.bottom - 1);
  }
}

void
MapItemListWidget::OnActivateItem(unsigned index)
{
  details_button->Click();
}

inline void
MapItemListWidget::OnGotoClicked()
{
  if (protected_task_manager == NULL)
    return;

  unsigned index = GetCursorIndex();
  auto const &item = *list[index];

  assert(item.type == MapItem::WAYPOINT);

  auto waypoint = ((const WaypointMapItem &)item).waypoint;
  protected_task_manager->DoGoto(std::move(waypoint));
  cancel_button->Click();
}

inline void
MapItemListWidget::OnAckClicked()
{
  const AirspaceMapItem &as_item = *(const AirspaceMapItem *)list[GetCursorIndex()];
  GetAirspaceWarnings()->AcknowledgeDay(*as_item.airspace);
  UpdateButtons();
}

void
MapItemListWidget::OnAction(int id)
{
  switch (id) {
  case SETTINGS:
    ShowMapItemListSettingsDialog();
    Profile::Save();
    this->bsetting_done = true;
    cancel_button->Click();
    break;
  case GOTO:
    OnGotoClicked();
    break;

  case ACK:
    OnAckClicked();
    break;
  }
}

static int
ShowMapItemListDialogNow(const MapItemList &list,
                         const DialogLook &dialog_look, const MapLook &look,
                         const TrafficLook &traffic_look,
                         const FinalGlideBarLook &final_glide_look,
                         const MapSettings &settings)
{
  MapItemListWidget widget(list, dialog_look, look, traffic_look,
                           final_glide_look, settings);
  WidgetDialog dialog(dialog_look);
  dialog.CreateFull(UIGlobals::GetMainWindow(),
                    _("Map elements at this location"), &widget);
  widget.CreateButtons(dialog);
  dialog.EnableCursorSelection();

  int result =
      dialog.ShowModal() == mrOK ? (int)widget.GetCursorIndex() :
                                   MAPITEMLIST_CANCEL;
  dialog.StealWidget();

  if (widget.bsetting_done)
    return MAPITEMLIST_REBUILD; // mapitemlist setting was done... => rebuild

  return result;
}

static void
ShowMapItemDialog(const MapItem &item,
                  ProtectedAirspaceWarningManager *airspace_warnings)
{
  switch (item.type) {
  case MapItem::LOCATION:
  case MapItem::ARRIVAL_ALTITUDE:
  case MapItem::SELF:
  case MapItem::THERMAL:
#ifdef HAVE_SKYLINES_TRACKING
  case MapItem::SKYLINES_TRAFFIC:
#endif
    break;

  case MapItem::AIRSPACE:
    dlgAirspaceDetails(*((const AirspaceMapItem &)item).airspace,
                       airspace_warnings);
    break;
  case MapItem::WAYPOINT:
    dlgWaypointDetailsShowModal(((const WaypointMapItem &)item).waypoint, true,
                                true);
    break;
  case MapItem::TASK_OZ:
    dlgTargetShowModal(((const TaskOZMapItem &)item).index);
    break;
  case MapItem::TRAFFIC:
    dlgFlarmTrafficDetailsShowModal(((const TrafficMapItem &)item).id);
    break;

#ifdef HAVE_NOAA
  case MapItem::WEATHER:
    dlgNOAADetailsShowModal(((const WeatherStationMapItem &)item).station);
    break;
#endif

  case MapItem::OVERLAY:
    ShowWeatherDialog(_T("overlay"));
    break;

  case MapItem::RASP:
    ShowWeatherDialog(_T("rasp"));
    break;
  }
}

int
ShowMapItemListDialog(const MapItemList &list, const DialogLook &dialog_look,
                      const MapLook &look, const TrafficLook &traffic_look,
                      const FinalGlideBarLook &final_glide_look,
                      const MapSettings &settings,
                      ProtectedAirspaceWarningManager *airspace_warnings)
{
  int iret = MAPITEMLIST_CANCEL;
  switch (list.size()) {
  case 0:
    /* no map items in the list */
    return MAPITEMLIST_CANCEL;

  case 1:
    /* only one map item, show it */
    ShowMapItemDialog(*list[0], airspace_warnings);
    return 0;

  default:
    /* more than one map item: show a list */

    iret = ShowMapItemListDialogNow(list, dialog_look, look, traffic_look,
                                    final_glide_look, settings);
    /*
     * checking for right return value (for debugging purposes)
     */
    if (iret != MAPITEMLIST_CANCEL && iret != MAPITEMLIST_REBUILD) {
      assert(iret >= 0 && iret < (int)list.size());
    }

    if (iret >= 0)
      ShowMapItemDialog(*list[iret], airspace_warnings);

  }
  return iret;
}
