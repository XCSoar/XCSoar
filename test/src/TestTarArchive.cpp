// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "io/MemoryReader.hxx"
#include "io/StringOutputStream.hxx"
#include "io/TarArchive.hpp"
#include "TestUtil.hpp"

#include <span>
#include <stdexcept>
#include <string>

static void
TestRoundTripMaxNameLength()
{
  const std::string payload = "abc";
  const std::string name(100, 'a');

  MemoryReader input(std::as_bytes(std::span(payload.data(), payload.size())));
  StringOutputStream archive_output;
  TarWriter writer(archive_output);
  writer.Add(name, input, payload.size());
  writer.Finish();

  const std::string archive = archive_output.GetValue();
  MemoryReader archive_input(
    std::as_bytes(std::span(archive.data(), archive.size())));
  TarReader reader(archive_input);

  std::string parsed_name;
  uint64_t parsed_size = 0;
  ok1(reader.Next(parsed_name, parsed_size));
  ok1(parsed_name == name);
  ok1(parsed_size == payload.size());

  StringOutputStream extracted;
  reader.ReadData(extracted);
  ok1(extracted.GetValue() == payload);
  ok1(!reader.Next(parsed_name, parsed_size));
}

static void
TestRejectLongName()
{
  const std::string payload = "abc";
  const std::string name(101, 'b');

  MemoryReader input(std::as_bytes(std::span(payload.data(), payload.size())));
  StringOutputStream archive_output;
  TarWriter writer(archive_output);

  bool thrown = false;
  try {
    writer.Add(name, input, payload.size());
  } catch (const std::invalid_argument &) {
    thrown = true;
  }

  ok1(thrown);
}

int
main()
{
  plan_tests(6);
  TestRoundTripMaxNameLength();
  TestRejectLongName();
  return exit_status();
}
