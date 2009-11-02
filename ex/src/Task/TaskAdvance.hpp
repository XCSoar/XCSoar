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
  TaskAdvance():
    armed(false),
    mode(ADVANCE_AUTO)
    {};

  enum TaskAdvanceMode_t {
    ADVANCE_MANUAL =0,
    ADVANCE_AUTO,
    ADVANCE_ARM,
    ADVANCE_ARMSTART
  };

  void set_armed(const bool do_armed) 
    {
      armed = do_armed;
    }
  bool is_armed() const 
    {
      return armed;
    }
  bool toggle_armed()
    {
      armed = !armed;
      return armed;
    }

  bool ready_to_advance(const TaskPoint &tp,
                        const AIRCRAFT_STATE &state,
                        const bool x_enter, 
                        const bool x_exit) const;
 
private:
  bool mode_ready() const;
  bool armed;
  TaskAdvanceMode_t mode;
};


#endif
