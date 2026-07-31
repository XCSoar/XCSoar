// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDialogs.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Waypoint/WaypointDialogs.hpp"
#include "Dialogs/Message.hpp"
#include "Form/Panel.hpp"
#include "Form/Draw.hpp"
#include "Form/Frame.hpp"
#include "Form/Button.hpp"
#include "Form/CheckBox.hpp"
#include "Form/Edit.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "Widget/ManagedWidget.hpp"
#include "Widget/PanelWidget.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Font.hpp"
#include "Components.hpp"
#include "Units/Units.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Ordered/Points/OrderedTaskPoint.hpp"
#include "Engine/Task/Ordered/Points/ASTPoint.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Task/Factory/TaskPointFactoryType.hpp"
#include "Task/ObservationZones/LineSectorZone.hpp"
#include "Task/ObservationZones/CylinderZone.hpp"
#include "Task/ObservationZones/KeyholeZone.hpp"
#include "Task/TypeStrings.hpp"
#include "Gauge/TaskView.hpp"
#include "util/Compiler.h"
#include "UIGlobals.hpp"
#include "Look/MapLook.hpp"
#include "Look/DialogLook.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "ui/event/KeyCode.hpp"
#include "Widgets/CylinderZoneEditWidget.hpp"
#include "Widgets/SectorZoneEditWidget.hpp"
#include "Widgets/LineSectorZoneEditWidget.hpp"
#include "Widgets/KeyholeZoneEditWidget.hpp"
#include "DataComponents.hpp"

#ifdef ENABLE_OPENGL
#include "ui/canvas/opengl/Scissor.hpp"
#endif

namespace {

/* Window::Move asserts a non-empty rect; ApplyLayout replaces this
   once the OZ form's minimum height is known. */
constexpr PixelSize OZ_PREPARE_PLACEHOLDER{1, 1};

} // namespace

class TaskPointWidget final
  : public NullWidget,
    DataFieldListener,
    ObservationZoneEditWidget::Listener {

  struct Layout {
    PixelRect waypoint_panel;
    PixelRect waypoint_name;
    PixelRect waypoint_details, waypoint_remove, waypoint_relocate;

    PixelRect tp_panel;
    PixelRect type_field;
    PixelRect map, properties;
    PixelRect optional_starts, score_exit;

    /**
     * @param properties_height portrait-only: height of the active OZ
     *        form (0 when none); landscape ignores this and uses a
     *        fixed side column
     */
    Layout(PixelRect rc, const DialogLook &look,
           unsigned properties_height) noexcept;
  };

  OrderedTask &ordered_task;
  bool task_modified;
  unsigned active_index;
  bool loading_type = false;

  WidgetDialog &dialog;
  const DialogLook &look;
  PixelRect position{};

  PanelControl waypoint_panel;
  WndFrame waypoint_name;
  Button waypoint_details, waypoint_remove, waypoint_relocate;

  PanelControl tp_panel;
  WndProperty type_field;
  WndOwnerDrawFrame map;
  ManagedWidget properties_widget{nullptr};

  Button optional_starts;
  CheckBoxControl score_exit;

  Button *previous_button, *next_button;

public:
  TaskPointWidget(WidgetDialog &_dialog,
                  OrderedTask &_task, unsigned _index)
    :ordered_task(_task), task_modified(false), active_index(_index),
     dialog(_dialog), look(dialog.GetLook()),
     waypoint_name(look),
     type_field(look) {}

  bool IsModified() const noexcept {
    return task_modified;
  }

  void CreateButtons() {
    previous_button = dialog.AddSymbolButton("<", [this](){
      OnPreviousClicked();
    });

    next_button = dialog.AddSymbolButton(">", [this](){
      OnNextClicked();
    });
  }

private:
  [[nodiscard]]
  unsigned GetPropertiesFormHeight() noexcept {
    if (!properties_widget.IsPrepared())
      return 0;

    return properties_widget.Get()->GetMinimumSize().height;
  }

  Layout MakeLayout(const PixelRect &rc) noexcept {
    return Layout(rc, look, GetPropertiesFormHeight());
  }

  void MoveChildren(const Layout &layout) noexcept {
    waypoint_name.Move(layout.waypoint_name);
    waypoint_details.Move(layout.waypoint_details);
    waypoint_remove.Move(layout.waypoint_remove);
    waypoint_relocate.Move(layout.waypoint_relocate);

    type_field.Move(layout.type_field);
    map.Move(layout.map);
    if (layout.properties.GetHeight() > 0) {
      properties_widget.Move(layout.properties);
      if (!properties_widget.IsVisible())
        properties_widget.Show();
    } else if (properties_widget.IsVisible())
      properties_widget.Hide();
    optional_starts.Move(layout.optional_starts);
    score_exit.Move(layout.score_exit);
  }

  void ApplyLayout(const PixelRect &rc) noexcept {
    position = rc;
    const Layout layout = MakeLayout(rc);
    waypoint_panel.Move(layout.waypoint_panel);
    tp_panel.Move(layout.tp_panel);
    MoveChildren(layout);
  }

  void RecreateOzForm() noexcept;
  void RefreshControls() noexcept;
  void RefreshView() noexcept;
  void LoadTypeField() noexcept;
  bool ReadValues();

  void PaintMap(Canvas &canvas, const PixelRect &rc);

  void OnDetailsClicked();
  void OnRemoveClicked();
  void OnRelocateClicked();
  void OnPreviousClicked();
  void OnNextClicked();
  void OnOptionalStartsClicked();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  bool Save(bool &changed) noexcept override {
    ReadValues();
    changed = task_modified;
    return true;
  }

  void Show(const PixelRect &rc) noexcept override {
    position = rc;
    const Layout layout = MakeLayout(rc);
    waypoint_panel.MoveAndShow(layout.waypoint_panel);
    tp_panel.MoveAndShow(layout.tp_panel);
    MoveChildren(layout);
  }

  void Hide() noexcept override {
    waypoint_panel.Hide();
    tp_panel.Hide();
  }

  void Move(const PixelRect &rc) noexcept override {
    ApplyLayout(rc);
  }

  /* Like #TargetWidget: Left/Right flip the active task point (< / >). */
  bool KeyPress(unsigned key_code) noexcept override;

private:
  /* virtual methods from DataFieldListener */
  void OnModified(DataField &df) noexcept override;

  /* virtual methods from class ObservationZoneEditWidget::Listener */
  void OnModified(ObservationZoneEditWidget &widget) noexcept override;
};

TaskPointWidget::Layout::Layout(PixelRect rc, const DialogLook &look,
                                unsigned properties_height) noexcept
{
  const unsigned padding = ::Layout::GetTextPadding();
  const unsigned font_height = look.text_font.GetHeight();
  const unsigned button_height = ::Layout::GetMaximumControlHeight();
  /* Decide before cutting the waypoint header / Type / action
     buttons: those shrink the map area until it is often wider than
     tall even on a portrait dialog, which wrongly kept the side form. */
  const bool landscape = rc.GetWidth() > rc.GetHeight();

  waypoint_panel = rc.CutTopSafe(font_height + button_height + 5 * padding)
    .WithPadding(padding);

  auto waypoint_rc = PixelRect{waypoint_panel.GetSize()}.WithPadding(padding);

  waypoint_name = waypoint_rc.CutTopSafe(font_height + 2 * padding);

  auto waypoint_buttons = waypoint_rc;
  waypoint_buttons.top += padding;

  waypoint_details = waypoint_remove = waypoint_relocate = waypoint_buttons;
  waypoint_details.right = waypoint_remove.left =
    (2 * waypoint_buttons.left + waypoint_buttons.right) / 3;
  waypoint_remove.right = waypoint_relocate.left =
    (waypoint_buttons.left + 2 * waypoint_buttons.right) / 3;

  tp_panel = rc.WithPadding(padding);

  auto tp_rc = PixelRect{tp_panel.GetSize()}.WithPadding(padding);

  type_field = tp_rc.CutTopSafe(button_height);

  PixelRect buttons_rc = tp_rc.CutBottomSafe(button_height);
  optional_starts = score_exit = buttons_rc;

  map = tp_rc;
  if (properties_height > 0) {
    if (landscape) {
      /* landscape: OZ form beside the map */
      const unsigned landscape_oz_width = ::Layout::Scale(120);
      properties = map.CutRightSafe(landscape_oz_width);
    } else {
      /* portrait: only as tall as the active OZ fields */
      properties = map.CutBottomSafe(std::min(properties_height,
                                              tp_rc.GetHeight() / 2));
    }
  } else {
    /* No OZ form (e.g. fixed FAI sector): map keeps the full area.
       Leave properties empty; MoveChildren hides the widget. */
    properties.SetEmpty();
  }
}

void
TaskPointWidget::Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept
{
  position = rc;
  const Layout layout = MakeLayout(rc);

  WindowStyle panel_style;
  panel_style.Hide();
  panel_style.Border();
  panel_style.ControlParent();

  WindowStyle button_style;
  button_style.TabStop();

  waypoint_panel.Create(parent, look, layout.waypoint_panel, panel_style);
  waypoint_name.Create(waypoint_panel, layout.waypoint_name);
  waypoint_details.Create(waypoint_panel, look.button, _("Details"),
                          layout.waypoint_details,
                          button_style, [this](){ OnDetailsClicked(); });
  waypoint_remove.Create(waypoint_panel, look.button, _("Remove"),
                         layout.waypoint_remove,
                         button_style, [this](){ OnRemoveClicked(); });
  waypoint_relocate.Create(waypoint_panel, look.button, _("Relocate"),
                           layout.waypoint_relocate,
                           button_style, [this](){ OnRelocateClicked(); });

  tp_panel.Create(parent, look, layout.tp_panel, panel_style);

  /* Same control shape as RowFormWidget::AddEnum in config panels. */
  type_field.Create(tp_panel, layout.type_field, _("Type"),
                    0, button_style);
  type_field.SetCaptionWidth(type_field.GetRecommendedCaptionWidth());
  type_field.SetDataField(new DataFieldEnum(this));
  map.Create(tp_panel, layout.map, WindowStyle(),
             [this](Canvas &canvas, const PixelRect &rc){
               PaintMap(canvas, rc);
             });

  properties_widget.Initialise(tp_panel,
                               PixelRect{OZ_PREPARE_PLACEHOLDER});
  optional_starts.Create(tp_panel, look.button, _("Enable Alternate Starts"),
                         layout.optional_starts, button_style,
                         [this](){ OnOptionalStartsClicked(); });
  score_exit.Create(tp_panel, look, _("Score exit"),
                    layout.score_exit, button_style, {});

  RefreshView();
}

void
TaskPointWidget::Unprepare() noexcept
{
  properties_widget.Clear();
}

static std::unique_ptr<ObservationZoneEditWidget>
CreateObservationZoneEditWidget(ObservationZonePoint &oz, bool is_fai_general)
{
  switch (oz.GetShape()) {
  case ObservationZone::Shape::SECTOR:
  case ObservationZone::Shape::ANNULAR_SECTOR:
  case ObservationZone::Shape::SYMMETRIC_SECTOR:
    return std::make_unique<SectorZoneEditWidget>((SectorZone &)oz);

  case ObservationZone::Shape::LINE:
    return std::make_unique<LineSectorZoneEditWidget>((LineSectorZone &)oz, !is_fai_general);

  case ObservationZone::Shape::CYLINDER:
    return std::make_unique<CylinderZoneEditWidget>((CylinderZone &)oz, !is_fai_general);

  case ObservationZone::Shape::CUSTOM_KEYHOLE:
    return std::make_unique<KeyholeZoneEditWidget>((KeyholeZone &)oz);

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
TaskPointWidget::RecreateOzForm() noexcept
{
  OrderedTaskPoint &tp = ordered_task.GetPoint(active_index);

  properties_widget.Clear();

  ObservationZonePoint &oz = tp.GetObservationZone();
  const bool is_fai_general =
    ordered_task.GetFactoryType() == TaskFactoryType::FAI_GENERAL;
  auto new_oz = CreateObservationZoneEditWidget(oz, is_fai_general);
  if (new_oz != nullptr) {
    new_oz->SetListener(this);
    properties_widget.Set(std::move(new_oz));
  } else
    properties_widget.Set(std::make_unique<PanelWidget>());

  /* Prepare at a placeholder size so GetMinimumSize() works for
     portrait layout; ApplyLayout assigns the real rect (or hides). */
  properties_widget.Move(PixelRect{OZ_PREPARE_PLACEHOLDER});
  properties_widget.Show();

  /* OZ editors are created after Alternates / Score exit, so keep
     those actions last in TabStop order (Up/Down reach Radius). */
  optional_starts.BringToBottom();
  score_exit.BringToBottom();

  ApplyLayout(position);
}

void
TaskPointWidget::RefreshControls() noexcept
{
  map.Invalidate();

  const OrderedTaskPoint &tp = ordered_task.GetPoint(active_index);

  LoadTypeField();

  previous_button->SetEnabled(active_index > 0);
  next_button->SetEnabled(active_index < (ordered_task.TaskSize() - 1));

  optional_starts.SetVisible(active_index == 0);
  if (!ordered_task.HasOptionalStarts())
    optional_starts.SetCaption(_("Enable Alternate Starts"));
  else {
    StaticString<50> tmp;
    tmp.Format("%s (%d)", _("Edit Alternates"),
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
    name_prefix_buffer = "Start: ";
    break;

  case TaskPointType::AST:
    type_buffer = _("Task point");
    name_prefix_buffer.Format("%d: ", active_index);
    break;

  case TaskPointType::AAT:
    type_buffer = _("Assigned area point");
    name_prefix_buffer.Format("%d: ", active_index);
    break;

  case TaskPointType::FINISH:
    type_buffer = _("Finish point");
    name_prefix_buffer = "Finish: ";
    break;

  default:
    gcc_unreachable();
  }

  dialog.SetCaption(type_buffer);

  StaticString<100> buffer;
  buffer.Format("%s %s", name_prefix_buffer.c_str(),
                tp.GetWaypoint().name.c_str());
  waypoint_name.SetText(buffer);
}

void
TaskPointWidget::RefreshView() noexcept
{
  RecreateOzForm();
  RefreshControls();
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

  return properties_widget.Save(task_modified);
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
                 basic.GetLocationOrInvalid(),
                 CommonInterface::GetMapSettings(),
                 look.task, look.airspace, look.overlay,
                 data_components->terrain.get(),
                 data_components->airspaces.get());
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
  dlgWaypointDetailsShowModal(data_components->waypoints.get(),
                              task_point.GetWaypointPtr(), false);
}

inline void
TaskPointWidget::OnRelocateClicked()
{
  const GeoPoint &gpBearing = active_index > 0
    ? ordered_task.GetPoint(active_index - 1).GetLocation()
    : CommonInterface::Basic().location;

  auto wp = ShowWaypointListDialog(*data_components->waypoints, gpBearing,
                                   &ordered_task, active_index);
  if (wp == nullptr)
    return;

  ordered_task.GetFactory().Relocate(active_index, std::move(wp));
  ordered_task.ClearName();
  ordered_task.UpdateGeometry();
  task_modified = true;
  RefreshView();
}

void
TaskPointWidget::LoadTypeField() noexcept
{
  loading_type = true;

  DataFieldEnum &df = *(DataFieldEnum *)type_field.GetDataField();
  df.ClearChoices();
  df.EnableItemHelp(true);

  AbstractTaskFactory &factory = ordered_task.GetFactory();
  const LegalPointSet valid = factory.GetValidTypes(active_index);
  for (unsigned i = 0; i < LegalPointSet::N; ++i) {
    const auto type = TaskPointFactoryType(i);
    if (!valid.Contains(type))
      continue;

    df.addEnumText(OrderedTaskPointName(type), (unsigned)type,
                   OrderedTaskPointDescription(type));
  }

  df.SetValue(factory.GetType(ordered_task.GetPoint(active_index)));
  type_field.RefreshDisplay();

  loading_type = false;
}

void
TaskPointWidget::OnModified(DataField &df) noexcept
{
  if (loading_type || &df != type_field.GetDataField())
    return;

  AbstractTaskFactory &factory = ordered_task.GetFactory();
  const auto &old_point = ordered_task.GetPoint(active_index);
  const auto type =
    TaskPointFactoryType(((DataFieldEnum &)df).GetValue());
  if (type == factory.GetType(old_point))
    return;

  auto point = factory.CreateMutatedPoint(old_point, type);
  if (point == nullptr || !factory.Replace(*point, active_index, true)) {
    LoadTypeField();
    return;
  }

  ordered_task.ClearName();
  ordered_task.UpdateGeometry();
  task_modified = true;
  RefreshView();
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
  if (dlgTaskOptionalStarts(*data_components->waypoints, ordered_task)) {
    ordered_task.ClearName();
    ordered_task.UpdateGeometry();
    task_modified = true;
    RefreshView();
  }
}

void
TaskPointWidget::OnModified([[maybe_unused]] ObservationZoneEditWidget &widget) noexcept
{
  ReadValues();
  map.Invalidate();
}

bool
TaskPointWidget::KeyPress(unsigned key_code) noexcept
{
  switch (key_code) {
  case KEY_LEFT:
    if (active_index == 0)
      return false;
    OnPreviousClicked();
    return true;

  case KEY_RIGHT:
    if (active_index >= ordered_task.TaskSize() - 1)
      return false;
    OnNextClicked();
    return true;
  }

  return false;
}

bool
dlgTaskPointShowModal(OrderedTask &task,
                      const unsigned index)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  TWidgetDialog<TaskPointWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           look, _("Waypoint"));

  dialog.SetWidget(dialog, task, index);
  dialog.GetWidget().CreateButtons();
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();

  return dialog.GetWidget().IsModified();
}
