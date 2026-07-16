// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskPropertiesPanel.hpp"
#include "Internal.hpp"
#include "Form/DataField/Enum.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Engine/Task/Factory/AbstractTaskFactory.hpp"
#include "Task/TypeStrings.hpp"
#include "Units/Units.hpp"
#include "Language/Language.hpp"
#include "Interface.hpp"

using namespace std::chrono;

enum Controls {
  TASK_TYPE,
  START_MODE,
  MIN_TIME,
  START_REQUIRES_ARM,
  START_SCORE_EXIT,
  START_OPEN_TIME,
  START_CLOSE_TIME,
  START_MAX_SPEED,
  START_MAX_HEIGHT,
  START_HEIGHT_REF,
  FINISH_MIN_HEIGHT,
  FINISH_HEIGHT_REF,
  PEV_START_WAIT_TIME,
  PEV_START_WINDOW,
};

static constexpr StaticEnumChoice start_mode_list[] = {
  { StartMode::NORMAL, N_("Normal Start"),
    N_("Use normal start rules.") },
  { StartMode::FAI_START_FINISH, N_("FAI start+finish"),
    N_("Use FAI start and finish rules.") },
  { StartMode::PEV, N_("Pilot Event (PEV) Start"),
    N_("Enable Pilot Event (PEV) start timing.") },
  { StartMode::POLISH, N_("Polish Start"),
    N_("Placeholder for Polish start timing.") },
  nullptr
};

static StartMode
GetStartMode(const OrderedTaskSettings &p) noexcept
{
  return p.start_constraints.start_mode;
}

static void
SetStartMode(OrderedTaskSettings &p, StartMode mode) noexcept
{
  switch (mode) {
  case StartMode::NORMAL:
    p.finish_constraints.fai_finish = false;
    p.start_constraints.fai_finish = false;
    p.start_constraints.pev_start_enabled = false;
    break;

  case StartMode::FAI_START_FINISH:
    p.finish_constraints.fai_finish = true;
    p.start_constraints.fai_finish = true;
    p.start_constraints.pev_start_enabled = false;
    break;

  case StartMode::PEV:
  case StartMode::POLISH:
    p.finish_constraints.fai_finish = false;
    p.start_constraints.fai_finish = false;
    p.start_constraints.pev_start_enabled = true;
    break;
  }
  p.start_constraints.start_mode = mode;
}

TaskPropertiesPanel::TaskPropertiesPanel(TaskManagerDialog &_dialog,
                                         std::unique_ptr<OrderedTask> &_active_task,
                                         bool *_task_modified) noexcept
  :RowFormWidget(_dialog.GetLook()), dialog(_dialog),
   ordered_task_pointer(_active_task),
   ordered_task(ordered_task_pointer.get()),
   task_changed(_task_modified) {}

void
TaskPropertiesPanel::RefreshView()
{
  const TaskFactoryType ftype = ordered_task->GetFactoryType();
  const OrderedTaskSettings &p = ordered_task->GetOrderedTaskSettings();

  const bool aat_types = ftype == TaskFactoryType::AAT ||
    ftype == TaskFactoryType::MAT;
  const StartMode start_mode = GetStartMode(p);
  const bool fai_start_finish = start_mode == StartMode::FAI_START_FINISH;
  const bool pev_start = start_mode == StartMode::PEV ||
    start_mode == StartMode::POLISH;

  SetRowVisible(MIN_TIME, aat_types);
  LoadValueDuration(MIN_TIME, p.aat_min_time);

  LoadValueEnum(START_MODE, start_mode);

  LoadValue(START_REQUIRES_ARM, p.start_constraints.require_arm);
  LoadValue(START_SCORE_EXIT, p.start_constraints.score_exit);

  LoadValue(START_OPEN_TIME, p.start_constraints.open_time_span.GetRoughStart());
  LoadValue(START_CLOSE_TIME, p.start_constraints.open_time_span.GetRoughEnd());

  SetRowVisible(START_MAX_SPEED, !fai_start_finish);
  LoadValue(START_MAX_SPEED, p.start_constraints.max_speed,
            UnitGroup::HORIZONTAL_SPEED);

  SetRowVisible(START_MAX_HEIGHT, !fai_start_finish);
  LoadValue(START_MAX_HEIGHT, double(p.start_constraints.max_height),
            UnitGroup::ALTITUDE);

  SetRowVisible(START_HEIGHT_REF, !fai_start_finish);
  LoadValueEnum(START_HEIGHT_REF, p.start_constraints.max_height_ref);

  SetRowVisible(FINISH_MIN_HEIGHT, !fai_start_finish);
  LoadValue(FINISH_MIN_HEIGHT, double(p.finish_constraints.min_height),
            UnitGroup::ALTITUDE);

  SetRowVisible(FINISH_HEIGHT_REF, !fai_start_finish);
  LoadValueEnum(FINISH_HEIGHT_REF, p.finish_constraints.min_height_ref);

  LoadValueEnum(TASK_TYPE, ftype);

  SetRowVisible(PEV_START_WAIT_TIME, !fai_start_finish && pev_start);
  LoadValueDuration(PEV_START_WAIT_TIME,
                    p.start_constraints.pev_start_wait_time);
  SetRowVisible(PEV_START_WINDOW, !fai_start_finish && pev_start);
  LoadValueDuration(PEV_START_WINDOW,
                    p.start_constraints.pev_start_window);

  dialog.InvalidateTaskView();

  // aat_min_time
  // finish_min_height
}

void
TaskPropertiesPanel::ReadValues()
{
  OrderedTaskSettings p = ordered_task->GetOrderedTaskSettings();
  bool changed = false;

  TaskFactoryType newtype = ordered_task->GetFactoryType();
  changed |= SaveValueEnum(TASK_TYPE, newtype);

  changed |= SaveValue(MIN_TIME, p.aat_min_time);

  if (SaveValue(START_REQUIRES_ARM, p.start_constraints.require_arm))
    changed = true;

  changed |= SaveValue(START_SCORE_EXIT, p.start_constraints.score_exit);

  RoughTime new_open = p.start_constraints.open_time_span.GetRoughStart();
  RoughTime new_close = p.start_constraints.open_time_span.GetRoughEnd();
  const bool start_open_modified = SaveValue(START_OPEN_TIME, new_open);
  const bool start_close_modified = SaveValue(START_CLOSE_TIME, new_close);
  if (start_open_modified || start_close_modified) {
    p.start_constraints.open_time_span = TimeSpan::FromRoughTimes(new_open, new_close);
    changed = true;
  }

  auto max_speed = Units::ToSysSpeed(GetValueFloat(START_MAX_SPEED));
  if (max_speed != p.start_constraints.max_speed) {
    p.start_constraints.max_speed = max_speed;
    changed = true;
  }

  unsigned max_height =
    iround(Units::ToSysAltitude(GetValueFloat(START_MAX_HEIGHT)));
  if (max_height != p.start_constraints.max_height) {
    p.start_constraints.max_height = max_height;
    changed = true;
  }

  changed |= SaveValueEnum(START_HEIGHT_REF,
                           p.start_constraints.max_height_ref);

  unsigned min_height =
    iround(Units::ToSysAltitude(GetValueFloat(FINISH_MIN_HEIGHT)));
  if (min_height != p.finish_constraints.min_height) {
    p.finish_constraints.min_height = min_height;
    changed = true;
  }

  changed |= SaveValueEnum(FINISH_HEIGHT_REF,
                           p.finish_constraints.min_height_ref);

  StartMode start_mode = GetStartMode(p);
  changed |= SaveValueEnum(START_MODE, start_mode);
  if (changed)
    SetStartMode(p, start_mode);

  changed |= SaveValue(PEV_START_WAIT_TIME,
                       p.start_constraints.pev_start_wait_time);
  changed |= SaveValue(PEV_START_WINDOW,
                       p.start_constraints.pev_start_window);

  if (changed) {
    ordered_task->SetOrderedTaskSettings(p);
  }

  *task_changed |= changed;
}

void
TaskPropertiesPanel::OnTaskTypeChange(DataFieldEnum &df)
{
  const DataFieldEnum &dfe = (const DataFieldEnum &)df;
  const TaskFactoryType newtype = (TaskFactoryType)dfe.GetValue();
  if (newtype != ordered_task->GetFactoryType()) {
    ReadValues();
    ordered_task->SetFactory(newtype);
    *task_changed =true;
  }
}

void
TaskPropertiesPanel::OnModified(DataField &df) noexcept
{
  if (IsDataField(TASK_TYPE, df))
    OnTaskTypeChange((DataFieldEnum &)df);
  else if (IsDataField(START_MODE, df))
    ReadValues();

  RefreshView();
}

void
TaskPropertiesPanel::Prepare([[maybe_unused]] ContainerWindow &parent,
                             [[maybe_unused]] const PixelRect &rc) noexcept
{
  DataFieldEnum *dfe = new DataFieldEnum(this);
  dfe->EnableItemHelp(true);
  const std::vector<TaskFactoryType> factory_types =
    ordered_task->GetFactoryTypes();
  for (unsigned i = 0; i < factory_types.size(); i++) {
    dfe->addEnumText(OrderedTaskFactoryName(factory_types[i]),
                     (unsigned)factory_types[i],
                     OrderedTaskFactoryDescription(factory_types[i]));
    if (factory_types[i] == ordered_task->GetFactoryType())
      dfe->SetValue(factory_types[i]);
  }
  Add(_("Task type"), _("Sets the behaviour for the current task."), dfe);

  AddEnum(_("Start mode"),
          _("Choose how the start is configured."),
          start_mode_list,
          (unsigned)GetStartMode(ordered_task->GetOrderedTaskSettings()),
          this);

  AddDuration(_("AAT min. time"), _("Minimum AAT task time in minutes."),
              {}, hours{10}, minutes{1}, minutes{3});

  AddBoolean(_("Arm start manually"),
             _("Configure whether the start must be armed manually or automatically."),
             false);

  AddBoolean(_("Score start exit"), nullptr, false);

  const RoughTimeDelta time_zone =
    CommonInterface::GetComputerSettings().utc_offset;
  AddRoughTime(_("Start open time"), nullptr,
               RoughTime::Invalid(), time_zone);
  AddRoughTime(_("Start close time"), nullptr,
               RoughTime::Invalid(), time_zone);

  AddFloat(_("Start max. speed"),
           _("Maximum speed allowed in start observation zone. Set to 0 for no limit."),
           "%.0f %s", "%.0f",
           0, 300, 5, false, 0);

  AddFloat(_("Start max. height"),
           _("Maximum height based on start height reference (AGL or MSL) while starting the task. Set to 0 for no limit."),
           "%.0f %s", "%.0f",
           0, 10000, 25, false, 0);

  static constexpr StaticEnumChoice altitude_reference_list[] = {
    { AltitudeReference::AGL, N_("AGL"),
      N_("Reference is the height above the task point."), },
    { AltitudeReference::MSL, N_("MSL"),
      N_("Reference is altitude above mean sea level."), },
    nullptr
  };

  AddEnum(_("Start height ref."),
          _("Reference used for start max height rule."),
          altitude_reference_list);

  AddFloat(_("Finish min. height"),
           _("Minimum height based on finish height reference (AGL or MSL) while finishing the task. Set to 0 for no limit."),
           "%.0f %s", "%.0f",
           0, 10000, 25, false, 0);

  AddEnum(_("Finish height ref."),
          _("Reference used for finish min height rule."),
          altitude_reference_list);

  AddDuration(_("PEV start wait time"),
              _("Wait time in minutes after Pilot Event (PEV) and before start gate opens. "
                "0 means start opens immediately."),
              {}, minutes{10}, minutes{1}, {});
  AddDuration(_("PEV start window"),
              _("Number of minutes the start remains open after Pilot Event (PEV) and PEV wait time. "
                "0 means start will never close after it opens."),
              minutes{0}, minutes{10}, minutes{1}, {});
}

void
TaskPropertiesPanel::ReClick() noexcept
{
  dialog.TaskViewClicked();
}

void
TaskPropertiesPanel::Show(const PixelRect &rc) noexcept
{
  ordered_task = ordered_task_pointer.get();
  orig_taskType = ordered_task->GetFactoryType();

  RefreshView();

  RowFormWidget::Show(rc);
}

bool
TaskPropertiesPanel::Leave() noexcept
{
  ReadValues();
  if (orig_taskType != ordered_task->GetFactoryType()) {
    if (ordered_task->GetFactory().MutateTPsToTaskType())
      ordered_task->UpdateGeometry();
  }

  return true;
}
