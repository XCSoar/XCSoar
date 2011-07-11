/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef DEVICE_BLACKBOARD_H
#define DEVICE_BLACKBOARD_H

#include "Blackboard.hpp"
#include "SettingsComputerBlackboard.hpp"
#include "BasicComputer.hpp"
#include "Device/Simulator.hpp"
#include "Device/List.hpp"
#include "Thread/Mutex.hpp"

#include <cassert>

class GlidePolar;

/**
 * Blackboard used by com devices: can write NMEA_INFO, reads DERIVED_INFO.
 * Also provides methods to emulate device updates e.g. from logger replay
 * 
 * The DeviceBlackboard is used as the global ground truth-state
 * since it is accessed quickly with only one mutex
 */
class DeviceBlackboard:
  public BaseBlackboard,
  public SettingsComputerBlackboard
{
  Validity last_location_available;

  Validity last_te_vario_available, last_netto_vario_available;
  unsigned last_vario_counter;

  Simulator simulator;

  NMEA_INFO state_last;
  BasicComputer computer;

  /**
   * Data from each physical device.
   */
  NMEA_INFO per_device_data[NUMDEV];

  /**
   * Merged data from the physical devices.
   */
  NMEA_INFO real_data;

  /**
   * Data from simulator.
   */
  NMEA_INFO simulator_data;

  /**
   * Data from replay.
   */
  NMEA_INFO replay_data;

public:
  Mutex mutex;

public:
  void Initialise();
  void ReadBlackboard(const DERIVED_INFO &derived_info);
  void ReadSettingsComputer(const SETTINGS_COMPUTER &settings);

protected:
  NMEA_INFO& SetBasic() { return gps_info; }

public:
  NMEA_INFO &SetRealState(unsigned i) {
    assert(i < NUMDEV);
    return per_device_data[i];
  }

  NMEA_INFO &SetSimulatorState() { return simulator_data; }
  NMEA_INFO &SetReplayState() { return replay_data; }

public:
  const NMEA_INFO &RealState() const { return real_data; }

  void SetStartupLocation(const GeoPoint &loc, const fixed alt);
  void SetLocation(const GeoPoint &loc, const fixed speed, const Angle bearing,
                   const fixed alt, const fixed baroalt, const fixed t);
  void ProcessSimulation();
  void StopReplay();
  void SetTrack(Angle val);
  void SetSpeed(fixed val);
  void SetAltitude(fixed alt);
  void SetQNH(fixed qnh);
  void SetMC(fixed mc);

  /**
   * Check the expiry time of the device connection with the wall
   * clock time.  This method locks the blackboard, i.e. it may be
   * called from any thread.
   *
   * @return true if the connection has just expired, false if the
   * connection status has not changed
   */
  bool expire_wall_clock();

  /**
   * Copy real_data or simulator_data or replay_data to gps_info.
   * Caller must lock the blackboard.
   */
  void Merge();

  void tick();

private:
// moved from GlideComputerAirData
  void ProcessFLARM();

  const NMEA_INFO& LastBasic() { return state_last; }
};

extern DeviceBlackboard device_blackboard;

#endif
