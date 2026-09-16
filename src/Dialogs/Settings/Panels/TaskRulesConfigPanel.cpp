// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "TaskRulesConfigPanel.hpp"
#include "ConfigListPanel.hpp"
#include "Form/DataField/Enum.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "Profile/Keys.hpp"
#include "Profile/Profile.hpp"

#include <cmath>

static constexpr StaticEnumChoice altitude_reference_list[] = {
  { AltitudeReference::AGL, N_("AGL"),
    N_("Reference is the height above the task point."), },
  { AltitudeReference::MSL, N_("MSL"),
    N_("Reference is altitude above mean sea level."), },
  nullptr
};

/** the choices of a speed rule, in the unit of the user */
static constexpr int SPEED_MAX = 300, SPEED_STEP = 5;

/**
 * The rules a task imposes at its start and its finish.  All of it is
 * for the expert user level.
 */
class TaskRulesConfigPanel final : public ConfigListPanel {
  double start_max_speed, start_max_speed_margin;
  unsigned start_max_height, start_max_height_margin;
  AltitudeReference start_height_ref;

  unsigned finish_min_height;
  AltitudeReference finish_height_ref;

  std::chrono::duration<unsigned> pev_start_wait_time, pev_start_window;

private:
  /**
   * Add an item which opens the choice of a speed, one choice per
   * #SPEED_STEP up to #SPEED_MAX in the unit of the user; the value
   * is in m/s.
   */
  void AddSpeedItem(const char *caption, const char *help,
                    double &value) noexcept;

protected:
  /* virtual methods from class ConfigListPanel */
  void LoadSettings() noexcept override;
  void Fill() noexcept override;

public:
  /* virtual methods from class Widget */
  bool Save(bool &changed) noexcept override;
};

void
TaskRulesConfigPanel::LoadSettings() noexcept
{
  const TaskBehaviour &task_behaviour =
    CommonInterface::GetComputerSettings().task;
  const OrderedTaskSettings &otb = task_behaviour.ordered_defaults;

  start_max_speed = otb.start_constraints.max_speed;
  start_max_speed_margin = task_behaviour.start_margins.max_speed_margin;
  start_max_height = otb.start_constraints.max_height;
  start_max_height_margin = task_behaviour.start_margins.max_height_margin;
  start_height_ref = otb.start_constraints.max_height_ref;

  finish_min_height = otb.finish_constraints.min_height;
  finish_height_ref = otb.finish_constraints.min_height_ref;

  pev_start_wait_time = otb.start_constraints.pev_start_wait_time;
  pev_start_window = otb.start_constraints.pev_start_window;
}

void
TaskRulesConfigPanel::AddSpeedItem(const char *caption, const char *help,
                                   double &value) noexcept
{
  AddItem(caption, [this, caption, help, &value](){
    int user_value = (int)std::lround(Units::ToUserSpeed(value));
    if (PickNumber(caption, help, 0, SPEED_MAX, SPEED_STEP, user_value,
                   [](StaticString<32> &s, int v){
                     s = FormatUserSpeed(Units::ToSysSpeed(v), false).c_str();
                   })) {
      value = Units::ToSysSpeed(user_value);
      Refresh();
    }
  }, {.value = FormatUserSpeed(value, false).c_str(), .chevron = true});
}

void
TaskRulesConfigPanel::Fill() noexcept
{
  if (!IsExpert())
    return;

  AddGroup(_("Start"));

  AddSpeedItem(_("Start max. speed"),
               _("Maximum speed allowed in start observation zone. Set to 0 for no limit."),
               start_max_speed);

  AddSpeedItem(_("Start max. speed margin"),
               _("Maximum speed above maximum start speed to tolerate. Set to 0 for no tolerance."),
               start_max_speed_margin);

  AddAltitudeItem(_("Start max. height"),
                  _("Maximum height based on start height reference (AGL or MSL) while starting the task. "
                    "Set to 0 for no limit."),
                  0, 10000, 50, start_max_height);

  AddAltitudeItem(_("Start max. height margin"),
                  _("Maximum height above maximum start height to tolerate. Set to 0 for no tolerance."),
                  0, 10000, 50, start_max_height_margin);

  AddEnumItem(_("Start height ref."),
              _("Reference used for start max height rule."),
              altitude_reference_list, start_height_ref);

  AddGroup();

  AddAltitudeItem(_("Finish min. height"),
                  _("Minimum height based on finish height reference (AGL or MSL) while finishing the task. "
                    "Set to 0 for no limit."),
                  0, 10000, 50, finish_min_height);

  AddEnumItem(_("Finish height ref."),
              _("Reference used for finish min height rule."),
              altitude_reference_list, finish_height_ref);

  AddGroup();

  AddDurationItem(_("PEV start wait time"),
                  _("Wait time in minutes after Pilot Event and before start gate opens. "
                    "0 means start opens immediately."),
                  0, 1800, 60, pev_start_wait_time);

  AddDurationItem(_("PEV start window"),
                  _("Number of minutes start remains open after Pilot Event and PEV wait time."
                    "0 means start will never close after it opens."),
                  0, 1800, 60, pev_start_window);
}

bool
TaskRulesConfigPanel::Save(bool &_changed) noexcept
{
  bool changed = false;

  TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;
  OrderedTaskSettings &otb = task_behaviour.ordered_defaults;
  StartConstraints &start = otb.start_constraints;

  changed |= Profile::Update(ProfileKeys::StartMaxSpeed,
                             start.max_speed, start_max_speed);

  changed |= Profile::Update(ProfileKeys::StartMaxSpeedMargin,
                             task_behaviour.start_margins.max_speed_margin,
                             start_max_speed_margin);

  changed |= Profile::Update(ProfileKeys::StartMaxHeight,
                             start.max_height, start_max_height);

  changed |= Profile::Update(ProfileKeys::StartMaxHeightMargin,
                             task_behaviour.start_margins.max_height_margin,
                             start_max_height_margin);

  changed |= Profile::Update(ProfileKeys::StartHeightRef,
                             start.max_height_ref, start_height_ref);

  changed |= Profile::Update(ProfileKeys::FinishMinHeight,
                             otb.finish_constraints.min_height,
                             finish_min_height);

  changed |= Profile::Update(ProfileKeys::FinishHeightRef,
                             otb.finish_constraints.min_height_ref,
                             finish_height_ref);

  changed |= Profile::Update(ProfileKeys::PEVStartWaitTime,
                             start.pev_start_wait_time, pev_start_wait_time);

  changed |= Profile::Update(ProfileKeys::PEVStartWindow,
                             start.pev_start_window, pev_start_window);

  _changed |= changed;

  return true;
}

std::unique_ptr<Widget>
CreateTaskRulesConfigPanel()
{
  return std::make_unique<TaskRulesConfigPanel>();
}
