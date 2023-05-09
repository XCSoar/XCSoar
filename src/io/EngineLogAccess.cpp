// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "EngineLogAccess.hpp"
#include "io/FileLineReader.hpp"
#include "io/KeyValueFileReader.hpp"
#include "io/KeyValueFileWriter.hpp"
#include "io/FileOutputStream.hxx"
#include "io/BufferedOutputStream.hxx"
#include "lib/fmt/ToBuffer.hxx"
#include "util/PrintException.hxx"
#include "LocalPath.hpp"

bool
EngineLogAccess::ReadLogFile()
{
  try{
    FileLineReaderA file(LocalPath(log_file_name));
    KeyValueFileReader kvreader(file);
    KeyValuePair pair;

    while (kvreader.Read(pair)) {
      SensorToLog(std::string(pair.key), std::atof(pair.value));
    }
  } catch (...){
    PrintException(std::current_exception());
    return false;
  }
  return true;
}

bool
EngineLogAccess::WriteLogFile()
{
  try{
    FileOutputStream file(LocalPath(log_file_name), FileOutputStream::Mode::CREATE);
    BufferedOutputStream buffered(file);
    KeyValueFileWriter kvwriter(buffered);

    for (auto const& entry : engine_log) {
      kvwriter.Write(entry.first.c_str(), std::to_string(entry.second).c_str());
    }

    buffered.Flush();
    file.Commit();
  } catch (...){
    PrintException(std::current_exception());
    return false;
  }
  return true;
}

bool
EngineLogAccess::WriteSingleUpdateToLogFile(std::string bt_mac, double engine_runtime)
{
  SensorToLog(bt_mac, engine_runtime);

  try{
    FileOutputStream file(LocalPath(log_file_name), FileOutputStream::Mode::APPEND_OR_CREATE);
    const auto line = FmtBuffer<64>("{}={}\n", bt_mac, engine_runtime);
    file.Write(line, strlen(line)); // @brief is 0 terminated by FmtBuffer implementation.
    file.Commit();
  } catch (...){
    PrintException(std::current_exception());
    return false;
  }
  return true;
}

void EngineLogAccess::SensorToLog(std::string bt_mac, double engine_runtime)
{
  auto result_pair = engine_log.insert({bt_mac, engine_runtime});

  // does entry for bt_mac exist, update entry?
  if (!result_pair.second) {
    result_pair.first->second = engine_runtime;
  }
}
