// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskDefaultsConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Engine/Task/Ordered/OrderedTask.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"
#include "Task/Factory/AbstractTaskFactory.hpp"
#include "Task/TypeStrings.hpp"

#include <cmath>
#include <vector>

static const char *const Caption_GateWidth = N_("Gate width");
static const char *const Caption_Radius = N_("Radius");

/**
 * The radii a task point may have, in the unit of the user: fine
 * below one unit, and coarser the larger the radius is.
 */
static constexpr double radius_choices[] = {
  0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9,
  1, 1.5, 2, 2.5, 3, 4, 5, 6, 7, 8, 9, 10,
  15, 20, 25, 30, 40, 50, 75, 100,
};

/**
 * Let the user pick the radius of a task point from #radius_choices;
 * the value is in metres.
 *
 * @return true if the value has changed
 */
static bool
PickRadius(const char *caption, const char *help, double &value) noexcept
{
  constexpr unsigned n = std::size(radius_choices);

  BasicStringBuffer<char, 32> captions[n];
  PickerChoice choices[n];

  /* the choice nearest to the value */
  const double user_value = Units::ToUserDistance(value);
  int current = 0;

  for (unsigned i = 0; i < n; ++i) {
    captions[i] =
      FormatUserDistance(Units::ToSysDistance(radius_choices[i]), true, 1);
    choices[i] = {captions[i].c_str()};

    if (std::fabs(radius_choices[i] - user_value) <
        std::fabs(radius_choices[current] - user_value))
      current = i;
  }

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0)
    return false;

  const double new_value = Units::ToSysDistance(radius_choices[picked]);
  if (new_value == value)
    return false;

  value = new_value;
  return true;
}

/**
 * Let the user pick the type of a task point from the ones the
 * factory allows, with the description of each.
 *
 * @return true if the value has changed
 */
static bool
PickPointType(const char *caption, const char *help,
              const LegalPointSet &legal,
              TaskPointFactoryType &value) noexcept
{
  std::vector<PickerChoice> choices;
  std::vector<TaskPointFactoryType> types;
  int current = -1;

  for (unsigned i = 0; i < LegalPointSet::N; ++i) {
    const TaskPointFactoryType type = TaskPointFactoryType(i);
    if (!legal.Contains(type))
      continue;

    if (type == value)
      current = types.size();

    choices.push_back({OrderedTaskPointName(type),
                       OrderedTaskPointDescription(type)});
    types.push_back(type);
  }

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0 || types[picked] == value)
    return false;

  value = types[picked];
  return true;
}

/**
 * Let the user pick the type of a task, with the description of each.
 *
 * @return true if the value has changed
 */
static bool
PickTaskType(const char *caption, const char *help,
             const std::vector<TaskFactoryType> &factory_types,
             TaskFactoryType &value) noexcept
{
  std::vector<PickerChoice> choices;
  int current = -1;

  for (unsigned i = 0; i < factory_types.size(); ++i) {
    if (factory_types[i] == value)
      current = i;

    choices.push_back({OrderedTaskFactoryName(factory_types[i]),
                       OrderedTaskFactoryDescription(factory_types[i])});
  }

  const int picked = PickChoice(caption, help, choices, current);
  if (picked < 0 || factory_types[picked] == value)
    return false;

  value = factory_types[picked];
  return true;
}

/**
 * What a new task is made of: the type and the size of its start,
 * its turn points and its finish, and the type of the task itself.
 */
class TaskDefaultsConfigPanel final : public ConfigListPanel {
  SectorDefaults sector_defaults;
  TaskFactoryType task_type;
  std::chrono::duration<unsigned> aat_min_time, optimise_targets_margin;

  /** the types a racing task allows */
  LegalPointSet start_types, turnpoint_types, finish_types;
  std::vector<TaskFactoryType> factory_types;

private:
  /**
   * Add the item which opens the choice of the type of a task
   * point, and the item of its radius below it.
   */
  void AddPointItems(const char *caption, const char *help,
                     const LegalPointSet &legal,
                     TaskPointFactoryType &type,
                     const char *radius_help, double &radius,
                     TaskPointFactoryType line_type) noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
TaskDefaultsConfigPanel::LoadSettings() noexcept
{
  const TaskBehaviour &task_behaviour =
    CommonInterface::GetComputerSettings().task;

  sector_defaults = task_behaviour.sector_defaults;
  task_type = task_behaviour.task_type_default;
  aat_min_time = task_behaviour.ordered_defaults.aat_min_time;
  optimise_targets_margin = task_behaviour.optimise_targets_margin;

  OrderedTask temptask(task_behaviour);
  temptask.SetFactory(TaskFactoryType::RACING);

  const AbstractTaskFactory &factory = temptask.GetFactory();
  start_types = factory.GetValidStartTypes();
  turnpoint_types = factory.GetValidIntermediateTypes();
  finish_types = factory.GetValidFinishTypes();
  factory_types = temptask.GetFactoryTypes();
}

void
TaskDefaultsConfigPanel::AddPointItems(const char *caption,
                                       const char *help,
                                       const LegalPointSet &legal,
                                       TaskPointFactoryType &type,
                                       const char *radius_help,
                                       double &radius,
                                       TaskPointFactoryType line_type) noexcept
{
  AddItem(caption, [this, caption, help, &legal, &type](){
    if (PickPointType(caption, help, legal, type))
      Refresh();
  }, {.value = OrderedTaskPointName(type), .chevron = true});

  /* a line has a width, everything else a radius */
  const char *radius_caption =
    gettext(type == line_type ? Caption_GateWidth : Caption_Radius);

  AddItem(radius_caption, [this, radius_caption, radius_help, &radius](){
    if (PickRadius(radius_caption, radius_help, radius))
      Refresh();
  }, {.value = FormatUserDistance(radius, true, 1).c_str(),
      .chevron = true});
}

void
TaskDefaultsConfigPanel::Fill() noexcept
{
  AddGroup();
  AddPointItems(_("Start point"),
                _("Default start type for new tasks you create."),
                start_types, sector_defaults.start_type,
                _("Default radius or gate width of the start zone for new tasks."),
                sector_defaults.start_radius,
                TaskPointFactoryType::START_LINE);

  AddGroup();
  AddPointItems(_("Finish point"),
                _("Default finish type for new tasks you create."),
                finish_types, sector_defaults.finish_type,
                _("Default radius or gate width of the finish zone in new tasks."),
                sector_defaults.finish_radius,
                TaskPointFactoryType::FINISH_LINE);

  AddGroup();
  AddPointItems(_("Turn point"),
                _("Default turn point type for new tasks you create."),
                turnpoint_types, sector_defaults.turnpoint_type,
                _("Default radius of turnpoint cylinders and sectors in new tasks."),
                sector_defaults.turnpoint_radius,
                /* a turn point is never a line */
                TaskPointFactoryType::COUNT);

  AddGroup();

  AddItem(_("Task"), [this](){
    if (PickTaskType(_("Task"),
                     _("Default task type for new tasks you create."),
                     factory_types, task_type))
      Refresh();
  }, {.value = OrderedTaskFactoryName(task_type), .chevron = true});

  AddDurationItem(_("AAT min. time"),
                  _("Default AAT min. time for new AAT tasks."),
                  60, 36000, 300, aat_min_time);

  if (IsExpert())
    AddDurationItem(_("Optimisation margin"),
                    _("Safety margin for AAT task optimisation. Optimisation "
                      "seeks to complete the task at the minimum time plus this margin time."),
                    0, 1800, 60, optimise_targets_margin);
}

bool
TaskDefaultsConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;
  SectorDefaults &live = task_behaviour.sector_defaults;

  changed |= Profile::Update(ProfileKeys::StartType,
                             live.start_type, sector_defaults.start_type);

  changed |= Profile::Update(ProfileKeys::StartRadius,
                             live.start_radius, sector_defaults.start_radius);

  changed |= Profile::Update(ProfileKeys::TurnpointType,
                             live.turnpoint_type,
                             sector_defaults.turnpoint_type);

  changed |= Profile::Update(ProfileKeys::TurnpointRadius,
                             live.turnpoint_radius,
                             sector_defaults.turnpoint_radius);

  changed |= Profile::Update(ProfileKeys::FinishType,
                             live.finish_type, sector_defaults.finish_type);

  changed |= Profile::Update(ProfileKeys::FinishRadius,
                             live.finish_radius,
                             sector_defaults.finish_radius);

  changed |= Profile::Update(ProfileKeys::TaskType,
                             task_behaviour.task_type_default, task_type);

  changed |= Profile::Update(ProfileKeys::AATMinTime,
                             task_behaviour.ordered_defaults.aat_min_time,
                             aat_min_time);

  changed |= Profile::Update(ProfileKeys::AATTimeMargin,
                             task_behaviour.optimise_targets_margin,
                             optimise_targets_margin);

  _changed |= changed;
  return true;
}

std::unique_ptr<Widget>
CreateTaskDefaultsConfigPanel()
{
  return std::make_unique<TaskDefaultsConfigPanel>();
}
