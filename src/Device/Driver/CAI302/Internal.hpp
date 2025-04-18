// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "Protocol.hpp"

#include <vector>
#include <cstdint>

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
  void CommandMode(OperationEnvironment &env);
  void DownloadMode(OperationEnvironment &env);
  void UploadMode(OperationEnvironment &env);

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
  void Reboot(OperationEnvironment &env);

  /**
   * Power off the CAI302 by sending the command "DIE".
   */
  void PowerOff(OperationEnvironment &env);

  /**
   * Start logging unconditionally.
   */
  void StartLogging(OperationEnvironment &env);

  /**
   * Stop logging unconditionally.
   */
  void StopLogging(OperationEnvironment &env);

  /**
   * Set audio volume 0 is loudest, 170 is silent.
   */
  void SetVolume(unsigned volume, OperationEnvironment &env);

  /**
   * Erase all waypoints.
   */
  void ClearPoints(OperationEnvironment &env);

  /**
   * Erase the pilot name.
   */
  void ClearPilot(OperationEnvironment &env);

  /**
   * Erase all log memory.
   */
  void ClearLog(OperationEnvironment &env);

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

  void CloseNavpoints(OperationEnvironment &env);
};
