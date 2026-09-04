// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "StateFile.hpp"

#include "DataFilePath.hpp"
#include "io/FileOutputStream.hxx"
#include "io/FileReader.hxx"
#include "io/StringOutputStream.hxx"
#include "json/Parse.hxx"
#include "json/Serialize.hxx"

#include <boost/json.hpp>

#include <span>
#include <string>
#include <utility>

static constexpr const char *STATE_FILENAME = "update-state-v1.json";

UpdateStateData
LoadUpdateState() noexcept
{
  try {
    FileReader file(ResolveCacheDataPath(STATE_FILENAME));
    const auto parsed = ParseUpdateStateJSON(Json::Parse(file));
    if (parsed)
      return *parsed;
  } catch (...) {
  }

  return {};
}

void
SaveUpdateState(const UpdateStateData &state) noexcept
try {
  StringOutputStream stream;
  Json::Serialize(stream, MakeUpdateStateJSON(state));
  const std::string payload = std::move(stream).GetValue();

  FileOutputStream file(CacheDataSavePath(STATE_FILENAME));
  file.Write(std::as_bytes(std::span{payload}));
  file.Commit();
} catch (...) {
}
