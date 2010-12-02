/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#ifndef TEST_AIRCRAFT_HPP
#define TEST_AIRCRAFT_HPP

#include <vector>

#include "test_debug.hpp"
#include "Task/TaskManager.hpp"


class AircraftSim {
public:
  AircraftSim(int _test_num, 
              TaskManager& task_manager,
              const AutopilotParameters &_parms,
              bool _goto_target=false);

  const AIRCRAFT_STATE& get_state() {
    return state;
  }
  void set_wind(const fixed speed, const Angle direction);

  void Start();
  void Stop();
  bool Update();

  void print(std::ostream &f4);
  fixed time();

  void set_speed_factor(fixed f) {
    speed_factor = f;
  }
  GeoPoint target();

private:
  bool far();
  fixed small_rand();
  void update_state();
  void update_mode();
  void integrate();

  fixed target_height();

  GeoPoint endpoint(const Angle& bear) const;
  void update_bearing();

  int test_num;
  Filter heading_filt;
  bool goto_target;
  fixed speed_factor;
  fixed climb_rate;
  bool short_flight;

  AIRCRAFT_STATE state, state_last;
  std::vector<GeoPoint> w;
  Angle bearing;
  unsigned awp;

  enum AcState {
    Climb = 0,
    Cruise,
    FinalGlide
  };
  
  AcState acstate; 
  TaskManager& task_manager;
  fixed random_mag;
  const AutopilotParameters &parms;
};

#endif
