#include "TaskAdvance.hpp"
#include "Tasks/BaseTask/TaskPoint.hpp"
#include "TaskPoints/StartPoint.hpp"
#include "TaskPoints/AATPoint.hpp"
#include "Tasks/BaseTask/IntermediatePoint.hpp"
#include "Navigation/Aircraft.hpp"

TaskAdvance::TaskAdvance():
    armed(false),
    mode(ADVANCE_AUTO)
{
}

bool 
TaskAdvance::ready_to_advance(const TaskPoint &tp,
                              const AIRCRAFT_STATE &state,
                              const bool x_enter, 
                              const bool x_exit) const
{
  if (!mode_ready())
    return false;

  if (dynamic_cast<const StartPoint*>(&tp)) {
    if (mode==ADVANCE_ARMSTART) {
      return x_exit && armed;
    } else {
      return x_exit;
    }
  }
  if (const AATPoint* ap = dynamic_cast<const AATPoint*>(&tp)) {
    /**
     * \todo
     * consider advancing only if close to target
     *  (will then work with auto-advance)
     */
    // return ap->isInSector(state);
    (void)ap;
    return x_exit;
  }
  if (const IntermediatePoint* ip = 
      dynamic_cast<const IntermediatePoint*>(&tp)) {
    return ip->isInSector(state);
  }
  // can't advance other types
  return false;
}

bool
TaskAdvance::mode_ready() const
{
  switch (mode) {
  case ADVANCE_MANUAL:
    return false;
  case ADVANCE_AUTO:
    return true;
  case ADVANCE_ARM:
    return armed;
  case ADVANCE_ARMSTART:
    return true;
  };
  return false;
}

