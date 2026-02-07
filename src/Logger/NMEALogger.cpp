// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Logger/NMEALogger.hpp"
#include "io/FileOutputStream.hxx"
#include "LocalPath.hpp"
#include "time/BrokenDateTime.hpp"
#include "system/Path.hpp"
#include "util/SpanCast.hxx"
#include "util/StaticString.hxx"

NMEALogger::NMEALogger() noexcept {}
NMEALogger::~NMEALogger() noexcept = default;

inline void
NMEALogger::Start()
{
  if (file != nullptr)
    return;

  BrokenDateTime dt = BrokenDateTime::NowUTC();
  assert(dt.IsPlausible());

  StaticString<64> name;
  name.Format("%04u-%02u-%02u_%02u-%02u.nmea",
              dt.year, dt.month, dt.day,
              dt.hour, dt.minute);

  const auto logs_path = MakeLocalPath("logs");

  const auto path = AllocatedPath::Build(logs_path, name);
  file = std::make_unique<FileOutputStream>(path,
                                            FileOutputStream::Mode::APPEND_OR_CREATE);
}

static void
WriteLine(OutputStream &os, std::string_view text)
{
  os.Write(AsBytes(text));

  static constexpr char newline = '\n';
  os.Write(ReferenceAsBytes(newline));
}

void
NMEALogger::Log(const char *text) noexcept
{
  if (!enabled)
    return;

  const std::lock_guard lock{mutex};

  try {
    Start();
    WriteLine(*file, text);
  } catch (...) {
  }
}
