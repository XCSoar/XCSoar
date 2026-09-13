// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Replay/IgcReplay.hpp"
#include "IGC/IGCParser.hpp"
#include "IGC/IGCFix.hpp"
#include "IGC/ApplyFixToNMEA.hpp"
#include "io/LineReader.hpp"
#include "NMEA/Info.hpp"

IgcReplay::IgcReplay(std::unique_ptr<NLineReader> &&_reader)
  :reader(std::move(_reader))
{
  extensions.clear();
}

IgcReplay::~IgcReplay()
{
}

inline bool
IgcReplay::ScanBuffer(const char *buffer, IGCFix &fix, NMEAInfo &basic)
{
  if (IGCParseFix(buffer, extensions, fix) && fix.gps_valid)
    return true;

  BrokenDate date;
  if (IGCParseDateRecord(buffer, date))
    basic.ProvideDate(date);
  else
    IGCParseExtensions(buffer, extensions);

  return false;
}

inline bool
IgcReplay::ReadPoint(IGCFix &fix, NMEAInfo &basic)
{
  char *buffer;

  while ((buffer = reader->ReadLine()) != nullptr) {
    if (ScanBuffer(buffer, fix, basic))
      return true;
  }

  return false;
}

bool
IgcReplay::Update(NMEAInfo &basic)
{
  IGCFix fix;

  while (true) {
    if (!ReadPoint(fix, basic))
      return false;

    if (fix.time.IsPlausible())
      break;
  }

  ApplyIGCFixToNMEA(fix, TimeStamp{fix.time.DurationSinceMidnight()},
                    std::nullopt, basic);

  return true;
}
