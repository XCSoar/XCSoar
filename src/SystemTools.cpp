// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemTools.hpp"
#include "io/LineReader.hpp"
#include "util/CharUtil.hxx"
#include "util/StringStrip.hxx"
#include "util/UTF8.hpp"

#include <stdexcept>
#include <utility>

namespace SystemTools {

static constexpr std::size_t MAX_ID_LENGTH = 64;
static constexpr std::size_t MAX_NAME_LENGTH = 80;
static constexpr std::size_t MAX_DESCRIPTION_LENGTH = 240;
static constexpr std::size_t MAX_INSTRUCTIONS_LENGTH = 1000;

bool
IsValidId(std::string_view id) noexcept
{
  if (id.empty() || id.size() > MAX_ID_LENGTH ||
      !IsAlphaNumericASCII(id.front()))
    return false;

  for (const char ch : id)
    if (!IsAlphaNumericASCII(ch) && ch != '-' && ch != '_' && ch != '.')
      return false;

  return true;
}

static bool
HasControlCharacter(std::string_view value) noexcept
{
  for (const unsigned char ch : value)
    if (ch < 0x20 || ch == 0x7f)
      return true;

  return false;
}

static void
SetText(std::string &destination, bool &defined, std::string_view value,
        std::size_t max_length)
{
  if (defined)
    throw std::runtime_error{"Duplicate system-tool field"};

  if (value.empty() || !ValidateUTF8(value) || HasControlCharacter(value) ||
      TruncateStringUTF8(value, max_length) != value.size())
    throw std::runtime_error{"Invalid system-tool text"};

  destination = value;
  defined = true;
}

static bool
IsLocalizedKey(std::string_view key, std::string_view base,
               std::string_view language) noexcept
{
  return !language.empty() &&
    key.size() == base.size() + language.size() + 2 &&
    key.starts_with(base) && key[base.size()] == '[' && key.back() == ']' &&
    key.substr(base.size() + 1, language.size()) == language;
}

Descriptor
ParseDescriptor(NLineReader &reader, std::string_view language)
{
  Descriptor result;
  bool have_name = false;
  bool have_description = false;
  bool have_instructions = false;
  bool have_mode = false;
  bool have_input = false;
  bool have_confirmation = false;
  bool have_localized_name = false;
  bool have_localized_description = false;
  bool have_localized_instructions = false;
  std::string localized_name;
  std::string localized_description;
  std::string localized_instructions;

  for (char *line; (line = reader.ReadLine()) != nullptr;) {
    std::string_view source = Strip(std::string_view{line});
    if (source.empty() || source.front() == '#')
      continue;

    const auto separator = source.find('=');
    if (separator == std::string_view::npos)
      throw std::runtime_error{"Malformed system-tool field"};

    const auto key = Strip(source.substr(0, separator));
    const auto value = Strip(source.substr(separator + 1));
    if (key.empty())
      throw std::runtime_error{"Empty system-tool field name"};

    if (key == "name")
      SetText(result.name, have_name, value, MAX_NAME_LENGTH);
    else if (key == "description")
      SetText(result.description, have_description, value,
              MAX_DESCRIPTION_LENGTH);
    else if (key == "instructions")
      SetText(result.instructions, have_instructions, value,
              MAX_INSTRUCTIONS_LENGTH);
    else if (IsLocalizedKey(key, "name", language))
      SetText(localized_name, have_localized_name, value, MAX_NAME_LENGTH);
    else if (IsLocalizedKey(key, "description", language))
      SetText(localized_description, have_localized_description, value,
              MAX_DESCRIPTION_LENGTH);
    else if (IsLocalizedKey(key, "instructions", language))
      SetText(localized_instructions, have_localized_instructions, value,
              MAX_INSTRUCTIONS_LENGTH);
    else if (key == "mode") {
      if (have_mode)
        throw std::runtime_error{"Duplicate system-tool mode"};

      if (value == "job")
        result.mode = Mode::JOB;
      else if (value == "exclusive")
        result.mode = Mode::EXCLUSIVE;
      else
        throw std::runtime_error{"Invalid system-tool mode"};

      have_mode = true;
    } else if (key == "input") {
      if (have_input)
        throw std::runtime_error{"Duplicate system-tool input"};

      if (value != "none")
        throw std::runtime_error{"Invalid system-tool input"};

      have_input = true;
    } else if (key == "confirmation") {
      if (have_confirmation)
        throw std::runtime_error{"Duplicate system-tool confirmation"};

      if (value == "required")
        result.confirmation = Confirmation::REQUIRED;
      else if (value == "none")
        result.confirmation = Confirmation::NONE;
      else
        throw std::runtime_error{"Invalid system-tool confirmation"};

      have_confirmation = true;
    }
  }

  if (!have_name || !have_description || !have_instructions || !have_mode)
    throw std::runtime_error{"Incomplete system-tool descriptor"};

  if (have_localized_name)
    result.name = std::move(localized_name);
  if (have_localized_description)
    result.description = std::move(localized_description);
  if (have_localized_instructions)
    result.instructions = std::move(localized_instructions);

  return result;
}

} // namespace SystemTools
