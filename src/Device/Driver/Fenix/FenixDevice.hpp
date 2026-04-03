// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Driver.hpp"
#include "Device/SettingsMap.hpp"
#include "thread/Mutex.hxx"
#include "util/StaticString.hxx"
#include "NMEA/InputLine.hpp"
#include "NMEA/Info.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Util/NMEAWriter.hpp"

#include <atomic>
#include <cstdint>




typedef enum  {
    FLIGHTS_IN_LOGBOOK,
    LOGBOOK_ITEM,
    IGC_FILE_NAME,
    DATE_OF_FLIGHT,
    FLIGHT_DURATION,
    PILOT_NAME,
    PILOT_SURNAME,
    FILE_SIZE,
    TAKE_OF_TIME,
    LANDING_TIME
    } ANS_LOGBOOK;

class FenixDevice : public AbstractDevice
{

private:

  Port &port;


public:
  FenixDevice(Port &_port) : port(_port) {}

  static bool LXWP0(NMEAInputLine &line, NMEAInfo &info);
  static bool LXWP1(NMEAInputLine &line, NMEAInfo &info);
  static bool LXWP2(NMEAInputLine &line, NMEAInfo &info);
  static bool LXWP3(NMEAInputLine &line, NMEAInfo &info);

  /* virtual methods from class Device */
  bool ParseNMEA(const char *line, struct NMEAInfo &info) override;
  bool ReadFlightList(RecordedFlightList &flight_list, OperationEnvironment &env) override;
  bool DownloadFlight(const RecordedFlightInfo &flight, Path path, OperationEnvironment &env) override;
  bool PutMacCready(double mac_cready, OperationEnvironment &env)  override;
  bool PutBugs(double bugs, OperationEnvironment &env) override;
  bool PutBallast(double fraction, double overload, OperationEnvironment &env) override;
  bool PutQNH(const AtmosphericPressure &pressure, OperationEnvironment &env) override;
  bool PutVolume([[maybe_unused]] unsigned volume, [[maybe_unused]] OperationEnvironment &env) override;
};
