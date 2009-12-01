#ifndef TEST_AIRCRAFT_HPP
#define TEST_AIRCRAFT_HPP

#include <vector>

#include "Task/TaskManager.hpp"


class AircraftSim {
public:
  AircraftSim(int _test_num, const TaskManager& task_manager,
              double random_mag,
              bool _goto_target=false);

  const AIRCRAFT_STATE& get_state() {
    return state;
  }
  GEOPOINT get_next() const;
  void set_wind(const double speed, const double direction);

  bool far(TaskManager &task_manager);
  fixed small_rand();
  void update_state(TaskManager &task_manager,
                    GlidePolar &glide_polar);
  void update_mode(TaskManager &task_manager);
  void integrate();

  bool advance(TaskManager &task_manager,
               GlidePolar &glide_polar);

#ifdef DO_PRINT
  void print(std::ostream &f4);
#endif
  fixed time();

  void set_speed_factor(fixed f) {
    speed_factor = f;
  }
  GEOPOINT target(TaskManager &task_manager);

private:
  fixed target_height(TaskManager &task_manager);

  GEOPOINT endpoint(const fixed bear) const;
  void update_bearing(TaskManager &task_manager);

  int test_num;
  Filter heading_filt;
  bool goto_target;
  fixed speed_factor;
  fixed climb_rate;
  bool short_flight;

  AIRCRAFT_STATE state, state_last;
  std::vector<GEOPOINT> w;
  fixed bearing;
  fixed sinkrate;
  unsigned awp;

  enum AcState {
    Climb = 0,
    Cruise,
    FinalGlide
  };
  
  AcState acstate; 

};

#endif
