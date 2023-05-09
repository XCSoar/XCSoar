// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "system/Path.hpp"

#include <tchar.h>
#include <map>
#include <string>

/**
 * @brief Runtime Log Access. For reading and writing engine runtime to a file.
 * The log file might have duplicated entries for the keys,
 * with different values. Key is the BT MAC of a engine sensor,
 * with engine runtime in seconds, as value.
 */
class EngineLogAccess{

/**
 * @brief Filename of the LocalPath file.
 */
const TCHAR *log_file_name = _T("engine_runtime.log");

/**
 * @brief The logfile data. BT Mac is key, engine runtime is value.
 */
std::map<std::string, double, std::less<>> engine_log {};

public:
  explicit EngineLogAccess() noexcept = default;

public:
  /**
   * @brief Read the whole engine_runtime.log into the engine_log map.
   * If there are dublicate entries in the log file, the highest
   * engine runtime value, for the particular bt mac, gets stored in the map.
   */
  bool ReadLogFile();

  /**
   * @brief Write the whole engine log map into the engine_runtime.log.
   * Overwrites the existing log.
   */
  bool WriteLogFile();

  /**
   * @brief Write one key value pair to the engine_runtime.log.
   * Might be a duplicate. Reading the log at app start takes care of
   * duplicates.
   * @param[in] bt_mac Bluetooth MAC of the engine sensor.
   * @param[in] engine_runtime Total runtime of the engine in seconds.
   */
  bool WriteSingleUpdateToLogFile(std::string bt_mac, double engine_runtime);

  /**
   * @brief Add or update engine sensor to the log map, identified by it's BT MAC.
   * If the sensor is already in the log map, the engine_runtime gets updated.
   * It does not get written to the engine_runtime.log file. Use
   * WriteSingleUpdateToLogFile, to get it added and written imediately.
   * @param[in] bt_mac Bluetooth MAC of the engine sensor.
   * @param[in] engine_runtime Total runtime of the engine in seconds.

   */
  void SensorToLog(std::string bt_mac, double engine_runtime);
};
