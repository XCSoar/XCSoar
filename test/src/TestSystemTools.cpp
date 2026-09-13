// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemTools.hpp"
#include "io/BufferedLineReader.hpp"
#include "io/MemoryReader.hxx"
#include "TestUtil.hpp"

#include <cstring>
#include <span>
#include <stdexcept>
#include <string>

static bool
Rejects(char *input)
{
  MemoryReader memory{std::as_bytes(std::span{input, std::strlen(input)})};
  BufferedLineReader reader{memory};
  try {
    SystemTools::ParseDescriptor(reader);
    return false;
  } catch (const std::runtime_error &) {
    return true;
  }
}

int
main()
{
  plan_tests(35);

  ok1(SystemTools::IsValidId("sensor-calibration"));
  ok1(SystemTools::IsValidId("vendor.tool_2"));
  ok1(!SystemTools::IsValidId(""));
  ok1(!SystemTools::IsValidId(".hidden"));
  ok1(!SystemTools::IsValidId("has/slash"));
  ok1(!SystemTools::IsValidId("has space"));

  char valid[] =
    "# Installed by the platform package\n"
    " name = Sensor calibration \n"
    "description=Calibrate the pressure sensors\n"
    "instructions=Keep the device still.\n"
    "mode=job\n"
    "input=none\n"
    "confirmation=required\n"
    "future-field=ignored\n";
  MemoryReader valid_memory{std::as_bytes(std::span{
    valid, sizeof(valid) - 1})};
  BufferedLineReader valid_reader{valid_memory};
  const auto descriptor = SystemTools::ParseDescriptor(valid_reader);
  ok1(descriptor.name == "Sensor calibration");
  ok1(descriptor.description == "Calibrate the pressure sensors");
  ok1(descriptor.instructions == "Keep the device still.");
  ok1(descriptor.mode == SystemTools::Mode::JOB);
  ok1(descriptor.confirmation == SystemTools::Confirmation::REQUIRED);

  char localized[] =
    "name[de]=Sensorkalibrierung\n"
    "description[de]=Sensoren kalibrieren\n"
    "instructions[de]=Nicht bewegen\n"
    "name=Sensor calibration\n"
    "description=Calibrate sensors\n"
    "instructions=Keep still\n"
    "mode=exclusive\n";
  MemoryReader localized_memory{std::as_bytes(std::span{
    localized, sizeof(localized) - 1})};
  BufferedLineReader localized_reader{localized_memory};
  const auto translated =
    SystemTools::ParseDescriptor(localized_reader, "de");
  ok1(translated.name == "Sensorkalibrierung");
  ok1(translated.description == "Sensoren kalibrieren");
  ok1(translated.instructions == "Nicht bewegen");
  ok1(translated.mode == SystemTools::Mode::EXCLUSIVE);

  char fallback[] =
    "name[de]=Sensorkalibrierung\n"
    "description[de]=Sensoren kalibrieren\n"
    "instructions[de]=Nicht bewegen\n"
    "name=Sensor calibration\n"
    "description=Calibrate sensors\n"
    "instructions=Keep still\n"
    "mode=job\n";
  MemoryReader fallback_memory{std::as_bytes(std::span{
    fallback, sizeof(fallback) - 1})};
  BufferedLineReader fallback_reader{fallback_memory};
  const auto untranslated =
    SystemTools::ParseDescriptor(fallback_reader, "fr");
  ok1(untranslated.name == "Sensor calibration");
  ok1(untranslated.description == "Calibrate sensors");
  ok1(untranslated.instructions == "Keep still");
  ok1(untranslated.confirmation == SystemTools::Confirmation::REQUIRED);

  char no_confirmation[] =
    "name=System information\n"
    "description=Show system information\n"
    "instructions=This action does not change settings.\n"
    "mode=job\n"
    "confirmation=none\n";
  MemoryReader no_confirmation_memory{std::as_bytes(std::span{
    no_confirmation, sizeof(no_confirmation) - 1})};
  BufferedLineReader no_confirmation_reader{no_confirmation_memory};
  ok1(SystemTools::ParseDescriptor(no_confirmation_reader).confirmation ==
      SystemTools::Confirmation::NONE);

  char destination_directory[] =
    "name=Back up system settings\n"
    "description=Create a settings archive\n"
    "instructions=Select a destination.\n"
    "mode=job\n"
    "input=destination-directory\n";
  ok1(Rejects(destination_directory));

  char existing_file[] =
    "name=Restore system settings\n"
    "description=Restore a settings archive\n"
    "instructions=Select an archive.\n"
    "mode=exclusive\n"
    "input=existing-file\n";
  ok1(Rejects(existing_file));

  char incomplete[] = "name=Missing fields\nmode=job\n";
  ok1(Rejects(incomplete));

  char malformed[] =
    "name=Malformed\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "this is not a field\n"
    "mode=job\n";
  ok1(Rejects(malformed));

  char duplicate[] =
    "name=First\n"
    "name=Second\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  ok1(Rejects(duplicate));

  char bad_mode[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=shell\n";
  ok1(Rejects(bad_mode));

  char bad_input[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n"
    "input=command-line\n";
  ok1(Rejects(bad_input));

  char duplicate_input[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n"
    "input=none\n"
    "input=existing-file\n";
  ok1(Rejects(duplicate_input));

  char bad_confirmation[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n"
    "confirmation=optional\n";
  ok1(Rejects(bad_confirmation));

  char duplicate_confirmation[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n"
    "confirmation=required\n"
    "confirmation=none\n";
  ok1(Rejects(duplicate_confirmation));

  char empty_text[] =
    "name=\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  ok1(Rejects(empty_text));

  char invalid_utf8[] =
    "name=Bad \xff\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  ok1(Rejects(invalid_utf8));

  std::string maximum_name =
    "name=" + std::string(80, 'x') + "\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  MemoryReader maximum_name_memory{std::as_bytes(std::span{
    maximum_name.data(), maximum_name.size()})};
  BufferedLineReader maximum_name_reader{maximum_name_memory};
  ok1(SystemTools::ParseDescriptor(maximum_name_reader).name.size() == 80);

  std::string excessive_name =
    "name=" + std::string(81, 'x') + "\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  ok1(Rejects(excessive_name.data()));

  char omitted_input[] =
    "name=Tool\n"
    "description=Description\n"
    "instructions=Instructions\n"
    "mode=job\n";
  MemoryReader omitted_input_memory{std::as_bytes(std::span{
    omitted_input, sizeof(omitted_input) - 1})};
  BufferedLineReader omitted_input_reader{omitted_input_memory};
  ok1(SystemTools::ParseDescriptor(omitted_input_reader).mode ==
      SystemTools::Mode::JOB);

  return exit_status();
}
