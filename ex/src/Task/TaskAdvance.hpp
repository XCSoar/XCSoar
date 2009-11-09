#ifndef TASKADVANCE_HPP
#define TASKADVANCE_HPP

class TaskPoint;
class StartPoint;
class IntermediatePoint;
class AATPoint;
struct AIRCRAFT_STATE;

class TaskAdvance
{
public:
/** 
 * Constructor.  Sets defaults to auto-mode
 */
  TaskAdvance()

  enum TaskAdvanceMode_t {
    ADVANCE_MANUAL =0,          /**< No automatic advance */
    ADVANCE_AUTO,               /**< Automatic, triggers as soon as condition satisfied */
    ADVANCE_ARM,                /**< Requires arming of trigger on each task point */
    ADVANCE_ARMSTART            /**< Requires arming of trigger before start, thereafter works as ADVANCE_AUTO */
  };

/** 
 * Set arming trigger
 * 
 * @param do_armed True to arm trigger, false to clear
 */
  void set_armed(const bool do_armed) 
    {
      armed = do_armed;
    }

/** 
 * Accessor for arm state
 * 
 * @return True if armed
 */
  bool is_armed() const 
    {
      return armed;
    }

/** 
 * Toggle arm state
 * 
 * @return Arm state after toggle
 */
  bool toggle_armed()
    {
      armed = !armed;
      return armed;
    }

/** 
 * Determine whether all conditions are satisfied for a turnpoint
 * to auto-advance based on condition of the turnpoint, transition
 * characteristics and advance mode.
 * 
 * @param tp The task point to check for satisfaction
 * @param state current aircraft state
 * @param x_enter whether this step transitioned enter to this tp
 * @param x_exit whether this step transitioned exit to this tp
 * 
 * @return true if this tp is ready to advance
 */
  bool ready_to_advance(const TaskPoint &tp,
                        const AIRCRAFT_STATE &state,
                        const bool x_enter, 
                        const bool x_exit) const;
 
private:
/** 
 * Determine whether mode allows auto-advance, without
 * knowledge about turnpoint or state characteristics
 * 
 * @return True if this mode allows auto-advance
 */
  bool mode_ready() const;

  bool armed;                   /**< arm state */
  TaskAdvanceMode_t mode;       /**< acive advance mode */
};


#endif
