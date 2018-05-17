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

#include "TaskDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Panel.hpp"
#include "Form/Draw.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Widget/DockWindow.hpp"
#include "Widget/PanelWidget.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Font.hpp"
#include "Components.hpp"
#include "Units/Units.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/TypeStrings.hpp"
#include "Gauge/TaskView.hpp"
#include "Compiler.h"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Widgets/CylinderZoneEditWidget.hpp"
#include "Widgets/SectorZoneEditWidget.hpp"
#include "Widgets/LineSectorZoneEditWidget.hpp"
#include "Widgets/KeyholeZoneEditWidget.hpp"

#ifdef ENABLE_OPENGL
#include "Screen/OpenGL/Scissor.hpp"
#endif

class TaskPointWidget final
  : public NullWidget,
    ObservationZoneEditWidget::Listener,
    ActionListener {
  enum Buttons {
    PREVIOUS, NEXT,
    DETAILS, REMOVE, RELOCATE,
    CHANGE_TYPE,
    OPTIONAL_STARTS,
    SCORE_EXIT,
  };

  struct Layout {
    PixelRect waypoint_panel;
    PixelRect waypoint_name;
    PixelRect waypoint_details, waypoint_remove, waypoint_relocate;

    PixelRect tp_panel;
    PixelRect type_label, change_type;
    PixelRect map, properties;
    PixelRect optional_starts, score_exit;

    explicit Layout(PixelRect rc, const DialogLook &look);
  };

  OrderedTask &ordered_task;
  bool task_modified;
  unsigned active_index;

  WidgetDialog &dialog;
  const DialogLook &look;

  PanelControl waypoint_panel;
  WndFrame waypoint_name;
  Button waypoint_details, waypoint_remove, waypoint_relocate;

  PanelControl tp_panel;
  WndFrame type_label;
  Button change_type;
  WndOwnerDrawFrame map;
  DockWindow properties_dock;

  Button optional_starts;
  CheckBoxControl score_exit;

  Button *previous_button, *next_button;

public:
  TaskPointWidget(WidgetDialog &_dialog,
                  OrderedTask &_task, unsigned _index)
    :ordered_task(_task), task_modified(false), active_index(_index),
     dialog(_dialog), look(dialog.GetLook()),
     waypoint_name(look),
     type_label(look) {}

  bool IsModified() const {
    return task_modified;
  }

  void CreateButtons() {
    previous_button = dialog.AddSymbolButton(_T("<"), *this, PREVIOUS);
    next_button = dialog.AddSymbolButton(_T(">"), *this, NEXT);
  }

private:
  void MoveChildren(const Layout &layout) {
    waypoint_name.Move(layout.waypoint_name);
    waypoint_details.Move(layout.waypoint_details);
    waypoint_remove.Move(layout.waypoint_remove);
    waypoint_relocate.Move(layout.waypoint_relocate);

    type_label.Move(layout.type_label);
    change_type.Move(layout.change_type);
    map.Move(layout.map);
    properties_dock.Move(layout.properties);
    optional_starts.Move(layout.optional_starts);
    score_exit.Move(layout.score_exit);
  }

  void RefreshView();
  bool ReadValues();

  void PaintMap(Canvas &canvas, const PixelRect &rc);

  void OnDetailsClicked();
  void OnRemoveClicked();
  void OnRelocateClicked();
  void OnTypeClicked();
  void OnPreviousClicked();
  void OnNextClicked();
  void OnOptionalStartsClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;

  void Unprepare() override {
    properties_dock.DeleteWidget();
  }

  bool Save(bool &changed) override {
    ReadValues();
    changed = task_modified;
    return true;
  }

  void Show(const PixelRect &rc) override {
    const Layout layout(rc, look);
    waypoint_panel.MoveAndShow(layout.waypoint_panel);
    tp_panel.MoveAndShow(layout.tp_panel);
    MoveChildren(layout);
  }

  void Hide() override {
    waypoint_panel.Hide();
    tp_panel.Hide();
  }

  void Move(const PixelRect &rc) override {
    const Layout layout(rc, look);
    waypoint_panel.Move(layout.waypoint_panel);
    tp_panel.Move(layout.tp_panel);
    MoveChildren(layout);
  }

private:
  /* virtual methods from class ObservationZoneEditWidget::Listener */
  void OnModified(ObservationZoneEditWidget &widget) override;

  /* virtual methods from class ActionListener */
  void OnAction(int id) override;
};

TaskPointWidget::Layout::Layout(PixelRect rc, const DialogLook &look)
{
  const unsigned padding = ::Layout::GetTextPadding();
  const unsigned font_height = look.text_font.GetHeight();
  const unsigned button_height = ::Layout::GetMaximumControlHeight();

  waypoint_panel = rc;
  waypoint_panel.left += padding;
  waypoint_panel.right -= padding;
  waypoint_panel.top += padding;
  waypoint_panel.bottom = waypoint_panel.top + 3 * padding
    + font_height + button_height;

  const PixelRect waypoint_rc(padding, padding,
                              waypoint_panel.GetWidth() - padding,
                              waypoint_panel.GetHeight() - padding);

  waypoint_name = waypoint_rc;
  waypoint_name.bottom = waypoint_name.top + font_height + 2 * padding;

  PixelRect waypoint_buttons(waypoint_rc);
  waypoint_buttons.top = waypoint_name.bottom + padding;

  waypoint_details = waypoint_remove = waypoint_relocate = waypoint_buttons;
  waypoint_details.right = waypoint_remove.left =
    (2 * waypoint_buttons.left + waypoint_buttons.right) / 3;
  waypoint_remove.right = waypoint_relocate.left =
    (waypoint_buttons.left + 2 * waypoint_buttons.right) / 3;

  tp_panel = rc;
  tp_panel.left += padding;
  tp_panel.right -= padding;
  tp_panel.top = waypoint_panel.bottom + padding;
  tp_panel.bottom -= padding;

  const PixelRect tp_rc(padding, padding,
                        tp_panel.GetWidth() - padding,
                        tp_panel.GetHeight() - padding);

  PixelRect type_rc = tp_rc;
  type_rc.bottom = type_rc.top + button_height;

  type_label = change_type = type_rc;
  type_label.right = change_type.left = type_rc.right
    - look.button.font->TextSize(_("Change Type")).cx - 3 * padding;

  PixelRect buttons_rc = tp_rc;
  buttons_rc.top = buttons_rc.bottom - button_height;
  optional_starts = score_exit = buttons_rc;

  map = tp_rc;
  map.top = type_rc.bottom;
  map.bottom = buttons_rc.top - padding;
  properties = map;

  map.right = properties.left = map.left + ::Layout::Scale(90);
}

void
TaskPointWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const Layout layout(rc, look);

  WindowStyle panel_style;
  panel_style.Hide();
  panel_style.Border();
  panel_style.ControlParent();

  WindowStyle button_style;
  button_style.TabStop();

  WindowStyle dock_style;
  dock_style.ControlParent();

  waypoint_panel.Create(parent, look, layout.waypoint_panel, panel_style);
  waypoint_name.Create(waypoint_panel, layout.waypoint_name);
  waypoint_details.Create(waypoint_panel, look.button, _("Details"),
                          layout.waypoint_details,
                          button_style, *this, DETAILS);
  waypoint_remove.Create(waypoint_panel, look.button, _("Remove"),
                         layout.waypoint_remove,
                         button_style, *this, REMOVE);
  waypoint_relocate.Create(waypoint_panel, look.button, _("Relocate"),
                           layout.waypoint_relocate,
                           button_style, *this, RELOCATE);

  tp_panel.Create(parent, look, layout.tp_panel, panel_style);

  type_label.Create(tp_panel, layout.type_label);
  change_type.Create(tp_panel, look.button, _("Change Type"),
                     layout.change_type,
                     button_style, *this, CHANGE_TYPE);
  map.Create(tp_panel, layout.map, WindowStyle(),
             [this](Canvas &canvas, const PixelRect &rc){
               PaintMap(canvas, rc);
             });
  properties_dock.Create(tp_panel, layout.properties, dock_style);
  optional_starts.Create(tp_panel, look.button, _("Enable Alternate Starts"),
                         layout.optional_starts, button_style,
                         *this, OPTIONAL_STARTS);
  score_exit.Create(tp_panel, look, _("Score exit"),
                    layout.score_exit, button_style,
                    *this, SCORE_EXIT);

  RefreshView();
}

void
TaskPointWidget::OnAction(int id)
{
    switch (id) {
    case PREVIOUS:
      OnPreviousClicked();
      break;

    case NEXT:
      OnNextClicked();
      break;

    case DETAILS:
      OnDetailsClicked();
      break;

    case REMOVE:
      OnRemoveClicked();
      break;

    case RELOCATE:
      OnRelocateClicked();
      break;

    case CHANGE_TYPE:
      OnTypeClicked();
      break;

    case OPTIONAL_STARTS:
      OnOptionalStartsClicked();
      break;
    }
}

static ObservationZoneEditWidget *
CreateObservationZoneEditWidget(ObservationZonePoint &oz, bool is_fai_general)
{
  switch (oz.GetShape()) {
  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::ANNULAR_SECTOR:
  case ObservationZone::Shape::SYMMETRIC_QUADRANT:
    return new SectorZoneEditWidget((SectorZone &)oz);

  case ObservationZone::Shape::LINE:
    return new LineSectorZoneEditWidget((LineSectorZone &)oz, !is_fai_general);

  case ObservationZone::Shape::CYLINDER:
    return new CylinderZoneEditWidget((CylinderZone &)oz, !is_fai_general);

  case ObservationZone::Shape::CUSTOM_KEYHOLE:
    return new KeyholeZoneEditWidget((KeyholeZone &)oz);

  case ObservationZone::Shape::FAI_SECTOR:
  case ObservationZone::Shape::DAEC_KEYHOLE:
  case ObservationZone::Shape::MAT_CYLINDER:
  case ObservationZone::Shape::BGAFIXEDCOURSE:
  case ObservationZone::Shape::BGAENHANCEDOPTION:
  case ObservationZone::Shape::BGA_START:
    break;
  }

  return nullptr;
}

void
TaskPointWidget::RefreshView()
{
  map.Invalidate();

  OrderedTaskPoint &tp = ordered_task.GetPoint(active_index);

  properties_dock.DeleteWidget();

  ObservationZonePoint &oz = tp.GetObservationZone();
  const bool is_fai_general =
    ordered_task.GetFactoryType() == TaskFactoryType::FAI_GENERAL;
  auto *properties_widget = CreateObservationZoneEditWidget(oz, is_fai_general);
  if (properties_widget != nullptr) {
    properties_widget->SetListener(this);
    properties_dock.SetWidget(properties_widget);
  } else
    properties_dock.SetWidget(new PanelWidget());

  type_label.SetCaption(OrderedTaskPointName(ordered_task.GetFactory().GetType(tp)));

  previous_button->SetEnabled(active_index > 0);
  next_button->SetEnabled(active_index < (ordered_task.TaskSize() - 1));

  optional_starts.SetVisible(active_index == 0);
  if (!ordered_task.HasOptionalStarts())
    optional_starts.SetCaption(_("Enable Alternate Starts"));
  else {
    StaticString<50> tmp;
    tmp.Format(_T("%s (%d)"), _("Edit Alternates"),
               ordered_task.GetOptionalStartPointCount());
    optional_starts.SetCaption(tmp);
  }

  if (tp.GetType() == TaskPointType::AST) {
    const ASTPoint &ast = (const ASTPoint &)tp;
    score_exit.Show();
    score_exit.SetState(ast.GetScoreExit());
  } else
    score_exit.Hide();

  StaticString<100> name_prefix_buffer, type_buffer;

  switch (tp.GetType()) {
  case TaskPointType::START:
    type_buffer = _("Start point");
    name_prefix_buffer = _T("Start: ");
    break;

  case TaskPointType::AST:
    type_buffer = _("Task point");
    name_prefix_buffer.Format(_T("%d: "), active_index);
    break;

  case TaskPointType::AAT:
    type_buffer = _("Assigned area point");
    name_prefix_buffer.Format(_T("%d: "), active_index);
    break;

  case TaskPointType::FINISH:
    type_buffer = _("Finish point");
    name_prefix_buffer = _T("Finish: ");
    break;

  default:
    gcc_unreachable();
  }

  dialog.SetCaption(type_buffer);

  {
    StaticString<100> buffer;
    buffer.Format(_T("%s %s"), name_prefix_buffer.c_str(),
                  tp.GetWaypoint().name.c_str());
    waypoint_name.SetCaption(buffer);
  }
}

bool
TaskPointWidget::ReadValues()
{
  OrderedTaskPoint &tp = ordered_task.GetPoint(active_index);

  if (tp.GetType() == TaskPointType::AST) {
    const bool new_score_exit = score_exit.GetState();

    ASTPoint &ast = (ASTPoint &)tp;

    if (new_score_exit != ast.GetScoreExit()) {
      ast.SetScoreExit(new_score_exit);
      ordered_task.ClearName();
      task_modified = true;
    }
  }

  return properties_dock.SaveWidget(task_modified);
}

void
TaskPointWidget::PaintMap(Canvas &canvas, const PixelRect &rc)
{
  const OrderedTaskPoint &tp = ordered_task.GetPoint(active_index);

#ifdef ENABLE_OPENGL
  /* enable clipping */
  GLCanvasScissor scissor(canvas);
#endif

  const MapLook &look = UIGlobals::GetMapLook();
  const NMEAInfo &basic = CommonInterface::Basic();
  PaintTaskPoint(canvas, rc, ordered_task, tp,
                 basic.location_available
                 ? basic.location : GeoPoint::Invalid(),
                 CommonInterface::GetMapSettings(),
                 look.task, look.airspace, look.overlay,
                 terrain, &airspace_database);
}

inline void
TaskPointWidget::OnRemoveClicked()
{
  if (ShowMessageBox(_("Remove task point?"), _("Task point"),
                  MB_YESNO | MB_ICONQUESTION) != IDYES)
    return;

  if (!ordered_task.GetFactory().Remove(active_index))
    return;

  ordered_task.ClearName();
  ordered_task.UpdateGeometry();
  task_modified = true;
  dialog.SetModalResult(mrCancel);
}

inline void
TaskPointWidget::OnDetailsClicked()
{
  const OrderedTaskPoint &task_point = ordered_task.GetPoint(active_index);
  dlgWaypointDetailsShowModal(task_point.GetWaypointPtr(), false);
}

inline void
TaskPointWidget::OnRelocateClicked()
{
  const GeoPoint &gpBearing = active_index > 0
    ? ordered_task.GetPoint(active_index - 1).GetLocation()
    : CommonInterface::Basic().location;

  auto wp = ShowWaypointListDialog(gpBearing, &ordered_task, active_index);
  if (wp == nullptr)
    return;

  ordered_task.GetFactory().Relocate(active_index, std::move(wp));
  ordered_task.ClearName();
  ordered_task.UpdateGeometry();
  task_modified = true;
  RefreshView();
}

inline void
TaskPointWidget::OnTypeClicked()
{
  if (dlgTaskPointType(ordered_task, active_index)) {
    ordered_task.ClearName();
    ordered_task.UpdateGeometry();
    task_modified = true;
    RefreshView();
  }
}

inline void
TaskPointWidget::OnPreviousClicked()
{
  if (active_index == 0 || !ReadValues())
    return;

  --active_index;
  RefreshView();
}

inline void
TaskPointWidget::OnNextClicked()
{
  if (active_index >= ordered_task.TaskSize() - 1 || !ReadValues())
    return;

  ++active_index;
  RefreshView();
}

/**
 * displays dlgTaskOptionalStarts
 * @param Sender
 */
inline void
TaskPointWidget::OnOptionalStartsClicked()
{
  if (dlgTaskOptionalStarts(ordered_task)) {
    ordered_task.ClearName();
    ordered_task.UpdateGeometry();
    task_modified = true;
    RefreshView();
  }
}

void
TaskPointWidget::OnModified(ObservationZoneEditWidget &widget)
{
  ReadValues();
  map.Invalidate();
}

bool
dlgTaskPointShowModal(OrderedTask &task,
                      const unsigned index)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(look);

  TaskPointWidget widget(dialog, task, index);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Waypoint"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  widget.CreateButtons();
  dialog.ShowModal();
  dialog.StealWidget();

  bool task_modified = widget.IsModified();
  return task_modified;
}
