/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#ifndef XCSOAR_CAI302_INTERNAL_HPP
#define XCSOAR_CAI302_INTERNAL_HPP

#include "Device/Driver.hpp"
#include "Protocol.hpp"

#include <vector>
#include <stdint.h>

/** 
 * Device driver for Cambridge Aero Instruments 302 
 */
class CAI302Device : public AbstractDevice {
  enum class Mode : uint8_t {
    UNKNOWN,
    NMEA,
    COMMAND,
    UPLOAD,
    DOWNLOAD,
  };

  const DeviceConfig &config;

  Port &port;

  Mode mode;

public:
  CAI302Device(const DeviceConfig &_config, Port &_port)
    :config(_config), port(_port), mode(Mode::UNKNOWN) {}

private:
  bool CommandMode(OperationEnvironment &env);
  bool DownloadMode(OperationEnvironment &env);
  bool UploadMode(OperationEnvironment &env);

  bool SetBaudRate(unsigned baud_rate, OperationEnvironment &env);

public:
  virtual void LinkTimeout() override;
  virtual bool EnableNMEA(OperationEnvironment &env) override;

  virtual bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  virtual bool PutMacCready(double mc, OperationEnvironment &env) override;
  virtual bool PutBugs(double bugs, OperationEnvironment &env) override;
  virtual bool PutBallast(double fraction, double overload,
                          OperationEnvironment &env) override;

  virtual bool Declare(const Declaration &declaration, const Waypoint *home,
                       OperationEnvironment &env) override;

  virtual bool ReadFlightList(RecordedFlightList &flight_list,
                              OperationEnvironment &env) override;
  virtual bool DownloadFlight(const RecordedFlightInfo &flight,
                              Path path,
                              OperationEnvironment &env) override;

public:
  bool EnableBulkMode(OperationEnvironment &env);
  bool DisableBulkMode(OperationEnvironment &env);

  bool ReadGeneralInfo(CAI302::GeneralInfo &data, OperationEnvironment &env);

  /**
   * Restart the CAI302 by sending the command "SIF 0 0".
   */
  bool Reboot(OperationEnvironment &env);

  /**
   * Power off the CAI302 by sending the command "DIE".
   */
  bool PowerOff(OperationEnvironment &env);

  /**
   * Start logging unconditionally.
   */
  bool StartLogging(OperationEnvironment &env);

  /**
   * Stop logging unconditionally.
   */
  bool StopLogging(OperationEnvironment &env);

  /**
   * Set audio volume 0 is loudest, 170 is silent.
   */
  bool SetVolume(unsigned volume, OperationEnvironment &env);

  /**
   * Erase all waypoints.
   */
  bool ClearPoints(OperationEnvironment &env);

  /**
   * Erase the pilot name.
   */
  bool ClearPilot(OperationEnvironment &env);

  /**
   * Erase all log memory.
   */
  bool ClearLog(OperationEnvironment &env);

  bool ReadPilotList(std::vector<CAI302::Pilot> &list,
                     unsigned &active_index,
                     OperationEnvironment &env);

  bool ReadActivePilot(CAI302::Pilot &pilot, OperationEnvironment &env);
  bool WriteActivePilot(const CAI302::Pilot &pilot, OperationEnvironment &env);

  bool WritePilot(unsigned index, const CAI302::Pilot &pilot,
                  OperationEnvironment &env);

  bool AddPilot(const CAI302::Pilot &pilot, OperationEnvironment &env);

  int ReadNavpointCount(OperationEnvironment &env);
  bool ReadNavpoint(unsigned index, CAI302::Navpoint &navpoint,
                    OperationEnvironment &env);

  bool WriteNavpoint(unsigned id, const Waypoint &wp,
                     OperationEnvironment &env);
};

#endif
