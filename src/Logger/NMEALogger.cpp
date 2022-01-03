/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "Logger/NMEALogger.hpp"
#include "io/FileOutputStream.hxx"
#include "LocalPath.hpp"
#include "time/BrokenDateTime.hpp"
#include "system/Path.hpp"
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
  name.Format(_T("%04u-%02u-%02u_%02u-%02u.nmea"),
              dt.year, dt.month, dt.day,
              dt.hour, dt.minute);

  const auto logs_path = MakeLocalPath(_T("logs"));

  const auto path = AllocatedPath::Build(logs_path, name);
  file = std::make_unique<FileOutputStream>(path,
                                            FileOutputStream::Mode::APPEND_OR_CREATE);
}

static void
WriteLine(OutputStream &os, std::string_view text)
{
  os.Write(text.data(), text.size());

  static constexpr char newline = '\n';
  os.Write(&newline, sizeof(newline));
}

void
NMEALogger::Log(const char *text) noexcept
{
  if (!enabled)
    return;

  std::lock_guard<Mutex> lock(mutex);

  try {
    Start();
    WriteLine(*file, text);
  } catch (...) {
  }
}
