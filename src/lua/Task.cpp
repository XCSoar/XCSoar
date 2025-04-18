// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Task.hpp"
#include "Chrono.hpp"
#include "MetaTable.hxx"
#include "Geo.hpp"
#include "Util.hxx"
#include "util/StringAPI.hxx"
#include "Interface.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Engine/Util/Gradient.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

using namespace std::chrono;

static int
l_task_index(lua_State *L)
{
  const char *name = lua_tostring(L, 2);
  if (name == nullptr)
    return 0;
  else if (StringIsEqual(name, "bearing")) {
    const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
    const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
    if (!task_stats.task_valid || !vector_remaining.IsValid() ||
        vector_remaining.distance <= 10) {
      return 0;
    }
    Lua::Push(L, vector_remaining.bearing);
  } else if (StringIsEqual(name, "bearing_diff")) {
      const NMEAInfo &basic = CommonInterface::Basic();
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
      if (!basic.track_available || !task_stats.task_valid ||
          !vector_remaining.IsValid() || vector_remaining.distance <= 10) {
        return 0;
      }      
      Lua::Push(L, vector_remaining.bearing - basic.track);
  } else if (StringIsEqual(name, "radial")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
      if (!task_stats.task_valid || !vector_remaining.IsValid() ||
          vector_remaining.distance <= 10) {
        return 0;
      }
      Lua::Push(L, vector_remaining.bearing.Reciprocal());
  } else if (StringIsEqual(name, "next_distance")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      const GeoVector &vector_remaining = task_stats.current_leg.vector_remaining;
      if (!task_stats.task_valid || !vector_remaining.IsValid()) 
        return 0;
     
      Lua::Push(L, vector_remaining.distance);
  } else if (StringIsEqual(name, "next_distance_nominal")) {
      const auto way_point = backend_components->protected_task_manager
        ? backend_components->protected_task_manager->GetActiveWaypoint()
        : nullptr;

      if (!way_point) return 0;
      const NMEAInfo &basic = CommonInterface::Basic();
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

      if (!task_stats.task_valid || !basic.location_available) return 0;

      const GeoVector vector(basic.location, way_point->location);

      if (!vector.IsValid()) return 0;
      Lua::Push(L, vector.distance);
  } else if (StringIsEqual(name, "next_ete")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      if (!task_stats.task_valid || !task_stats.current_leg.IsAchievable()) 
        return 0; 
      assert(task_stats.current_leg.time_remaining_now.count() >= 0);

      Lua::Push(L, task_stats.current_leg.time_remaining_now);
  } else if (StringIsEqual(name, "next_eta")) {
      const auto &task_stats = CommonInterface::Calculated().task_stats;
      const BrokenTime &now_local = CommonInterface::Calculated().date_time_local;

      if (!task_stats.task_valid || !task_stats.current_leg.IsAchievable() ||
          !now_local.IsPlausible()) {
        return 0;
      }

      const BrokenTime t = now_local +
        duration_cast<seconds>(task_stats.current_leg.solution_remaining.time_elapsed);

      Lua::Push(L, t.DurationSinceMidnight());
  } else if (StringIsEqual(name, "next_altitude_diff")) {    
      const auto &task_stats = CommonInterface::Calculated().task_stats;
      const auto &next_solution = task_stats.current_leg.solution_remaining;

      if (!task_stats.task_valid || !next_solution.IsAchievable())
        return 0;

      const auto &settings = CommonInterface::GetComputerSettings();
      auto altitude_difference = next_solution.SelectAltitudeDifference(settings.task.glide);
      Lua::Push(L, altitude_difference);
  } else if (StringIsEqual(name, "nextmc0_altitude_diff")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      if (!task_stats.task_valid || !task_stats.current_leg.solution_mc0.IsAchievable())
        return 0;

      const auto &settings = CommonInterface::GetComputerSettings();
      auto altitude_difference = task_stats.current_leg.solution_mc0.SelectAltitudeDifference(settings.task.glide);
      Lua::Push(L, altitude_difference);
  } else if (StringIsEqual(name, "next_altitude_require")) {
      const auto &task_stats = CommonInterface::Calculated().task_stats;
      const auto &next_solution = task_stats.current_leg.solution_remaining;
      if (!task_stats.task_valid || !next_solution.IsAchievable()) 
        return 0;

      Lua::Push(L, next_solution.GetRequiredAltitude());
  } else if (StringIsEqual(name, "next_altitude_arrival")) {
      const auto &basic = CommonInterface::Basic();
      const auto &task_stats = CommonInterface::Calculated().task_stats;
      const auto next_solution = task_stats.current_leg.solution_remaining;
      if (!basic.NavAltitudeAvailable() ||
          !task_stats.task_valid || !next_solution.IsAchievable()) {
        return 0;
      }

      Lua::Push(L, next_solution.GetArrivalAltitude(basic.nav_altitude));
  } else if (StringIsEqual(name, "next_gr")) {
      if (!CommonInterface::Calculated().task_stats.task_valid) 
        return 0;

      auto gradient = CommonInterface::Calculated().task_stats.current_leg.gradient;
      if (gradient <= 0) 
        return 0;     
      if (::GradientValid(gradient)) 
        Lua::Push(L, gradient);
      else 
        return 0;         
  } else if (StringIsEqual(name, "final_distance")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.task_stats;

      if (!task_stats.task_valid ||
          !task_stats.current_leg.vector_remaining.IsValid() ||
          !task_stats.total.remaining.IsDefined()) {
        return 0;
      }

      Lua::Push(L, task_stats.total.remaining.GetDistance());
  } else if (StringIsEqual(name, "final_ete")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

      if (!task_stats.task_valid || !task_stats.total.IsAchievable()) 
        return 0;
      assert(task_stats.total.time_remaining_now.count() >= 0);

      Lua::Push(L, task_stats.total.time_remaining_now);
  } else if (StringIsEqual(name, "final_eta")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      const BrokenTime &now_local = CommonInterface::Calculated().date_time_local;

      if (!task_stats.task_valid || !task_stats.total.IsAchievable() ||
          !now_local.IsPlausible()) 
        return 0;    

      const BrokenTime t = now_local +
        duration_cast<seconds>(task_stats.total.solution_remaining.time_elapsed);

      Lua::Push(L, t.DurationSinceMidnight());
  } else if (StringIsEqual(name, "final_altitude_diff")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      const auto &settings = CommonInterface::GetComputerSettings();
      if (!task_stats.task_valid || !task_stats.total.solution_remaining.IsAchievable())
        return 0;
      auto altitude_difference =
        task_stats.total.solution_remaining.SelectAltitudeDifference(settings.task.glide);

      Lua::Push(L, altitude_difference);
  } else if (StringIsEqual(name, "finalmc0_altitude_diff")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      const auto &settings = CommonInterface::GetComputerSettings();
      if (!task_stats.task_valid || !task_stats.total.solution_mc0.IsAchievable())
        return 0;
      auto altitude_difference =
        task_stats.total.solution_mc0.SelectAltitudeDifference(settings.task.glide);

      Lua::Push(L, altitude_difference);
  } else if (StringIsEqual(name, "final_altitude_require")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      if (!task_stats.task_valid ||
          !task_stats.total.solution_remaining.IsOk()) {
        return 0;
      }
   
      Lua::Push(L, task_stats.total.solution_remaining.GetRequiredAltitude());
  } else if (StringIsEqual(name, "task_speed")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      if (!task_stats.task_valid || !task_stats.total.travelled.IsDefined()) 
        return 0;

      Lua::Push(L, task_stats.total.travelled.GetSpeed());
  } else if (StringIsEqual(name, "task_speed_achieved")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      if (!task_stats.task_valid || !task_stats.total.remaining_effective.IsDefined()) 
        return 0;

      Lua::Push(L, task_stats.total.remaining_effective.GetSpeed());
  } else if (StringIsEqual(name, "task_speed_instant")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      if (!task_stats.task_valid)
        return -1;

      Lua::Push(L, task_stats.inst_speed_fast);
  } else if (StringIsEqual(name, "task_speed_hour")) {
      const WindowStats &window = CommonInterface::Calculated().task_stats.last_hour;
      if (!window.IsDefined())
        return 0;

      Lua::Push(L, window.speed);
  } else if (StringIsEqual(name, "final_gr")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      if (!task_stats.task_valid) 
        return 0;

      auto gradient = task_stats.total.gradient;

      if (gradient <= 0)
        return 0;
      if (::GradientValid(gradient))
        Lua::Push(L, gradient);
      else
        return 0;
  } else if (StringIsEqual(name, "aat_time")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.ordered_task_stats;
      const CommonStats &common_stats = calculated.common_stats;

      if (!task_stats.has_targets || !task_stats.total.IsAchievable()) 
        return 0;

      Lua::Push(L, common_stats.aat_time_remaining);
  } else if (StringIsEqual(name, "aat_time_delta")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.ordered_task_stats;
      const CommonStats &common_stats = calculated.common_stats;
      if (!task_stats.has_targets || !task_stats.total.IsAchievable()) 
        return 0;
   
      assert(task_stats.total.time_remaining_start.count() >= 0);

      auto diff = task_stats.total.time_remaining_start -
        common_stats.aat_time_remaining;

      Lua::Push(L, diff);
  } else if (StringIsEqual(name, "aat_distance")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.ordered_task_stats;

      if (!task_stats.has_targets || !task_stats.total.planned.IsDefined()) 
        return 0;
       
      Lua::Push(L, task_stats.total.planned.GetDistance());
  } else if (StringIsEqual(name, "aat_distance_max")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.ordered_task_stats;

      if (!task_stats.has_targets) 
        return 0;

      Lua::Push(L, task_stats.distance_max);
  } else if (StringIsEqual(name, "aat_distance_min")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.ordered_task_stats;

      if (!task_stats.has_targets) 
        return 0;

      Lua::Push(L, task_stats.distance_min);
  } else if (StringIsEqual(name, "aat_speed")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.ordered_task_stats;
      const CommonStats &common_stats = calculated.common_stats;

      if (!task_stats.has_targets || common_stats.aat_speed_target <= 0) 
        return 0;

      Lua::Push(L, common_stats.aat_speed_target);
  } else if (StringIsEqual(name, "aat_speed_max")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.ordered_task_stats;
      const CommonStats &common_stats = calculated.common_stats;

      if (!task_stats.has_targets || common_stats.aat_speed_max <= 0) 
        return 0;

      Lua::Push(L, common_stats.aat_speed_max);
  } else if (StringIsEqual(name, "aat_speed_min")) {
      const auto &calculated = CommonInterface::Calculated();
      const TaskStats &task_stats = calculated.ordered_task_stats;
      const CommonStats &common_stats = calculated.common_stats;

      if (!task_stats.has_targets ||
        !task_stats.task_valid || common_stats.aat_speed_min <= 0) 
        return 0;
    
      Lua::Push(L, common_stats.aat_speed_min);
  } else if (StringIsEqual(name, "time_under_max_height")) {
      const auto &calculated = CommonInterface::Calculated();
      const auto &task_stats = calculated.ordered_task_stats;
      const auto &common_stats = calculated.common_stats;
      const double maxheight = backend_components->protected_task_manager->GetOrderedTaskSettings().start_constraints.max_height;

      if (!task_stats.task_valid || maxheight <= 0
          || !backend_components->protected_task_manager
          || !common_stats.TimeUnderStartMaxHeight.IsDefined()) {
        return 0;
      }

      Lua::Push(L, (CommonInterface::Basic().time - common_stats.TimeUnderStartMaxHeight).count());
  } else if (StringIsEqual(name, "next_etevmg")) {
      const NMEAInfo &basic = CommonInterface::Basic();
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

      if (!basic.ground_speed_available || !task_stats.task_valid ||
          !task_stats.current_leg.remaining.IsDefined()) {  
        return 0;
      }
      const auto d = task_stats.current_leg.remaining.GetDistance();
      const auto v = basic.ground_speed;

      if (!task_stats.task_valid ||
          d <= 0 ||
          v <= 0) {
        return 0;
      }
   
      Lua::Push(L, d/v);
  } else if (StringIsEqual(name, "final_etevmg")) {
      const NMEAInfo &basic = CommonInterface::Basic();
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;

      if (!basic.ground_speed_available || !task_stats.task_valid ||
          !task_stats.total.remaining.IsDefined()) {
        return 0;
      }
      const auto d = task_stats.total.remaining.GetDistance();
      const auto v = basic.ground_speed;

      if (!task_stats.task_valid ||
          d <= 0 ||
          v <= 0) {
        return 0;
      }
 
      Lua::Push(L, d/v);
  } else if (StringIsEqual(name, "cruise_efficiency")) {
      const TaskStats &task_stats = CommonInterface::Calculated().task_stats;
      if (!task_stats.task_valid || !task_stats.start.HasStarted())
        return 0;

      Lua::Push(L, task_stats.cruise_efficiency);
  } else
    return 0;

  return 1;
}

void
Lua::InitTask(lua_State *L)
{
  lua_getglobal(L, "xcsoar");

  lua_newtable(L);

  MakeIndexMetaTableFor(L, RelativeStackIndex{-1}, l_task_index);

  lua_setfield(L, -2, "task");

  lua_pop(L, 1);
}
