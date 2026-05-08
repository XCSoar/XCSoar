// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "NumberParser.hpp"

#include <cstdlib>
#include <cstring>

/**
 * Parse an integer from an environment variable with validation.
 *
 * @param env_name the environment variable name
 * @param default_value the default value to use if env var is not set or
 * invalid
 * @param min_value minimum allowed value (inclusive), or 0 to disable
 * @param max_value maximum allowed value (inclusive), or 0 to disable
 * @return the parsed value, or default_value if parsing fails or value is
 * out of range
 */
[[gnu::pure]]
static inline int
GetEnvInt(const char *env_name, int default_value, int min_value = 0,
          int max_value = 0) noexcept
{
  const char *env_value = getenv(env_name);
  if (env_value == nullptr) return default_value;

  char *end;
  const int value = ParseInt(env_value, &end, 10);
  if (end == env_value || *end != '\0') return default_value;

  // Enable range validation when max_value != 0
  if (max_value != 0 && (value < min_value || value > max_value))
    return default_value;

  return value;
}

/**
 * Parse an integer from an environment variable with validation, returning
 * whether the variable was set and valid.
 *
 * @param env_name the environment variable name
 * @param value output parameter for the parsed value
 * @param min_value minimum allowed value (inclusive), or 0 to disable
 * @param max_value maximum allowed value (inclusive), or 0 to disable
 * @return true if the environment variable was set and valid, false otherwise
 */
[[gnu::pure]]
static inline bool
GetEnvInt(const char *env_name, int &value, int min_value = 0,
          int max_value = 0) noexcept
{
  const char *env_value = getenv(env_name);
  if (env_value == nullptr) return false;

  char *end;
  const int parsed = ParseInt(env_value, &end, 10);
  if (end == env_value || *end != '\0') return false;

  // Enable range validation when max_value != 0
  if (max_value != 0 && (parsed < min_value || parsed > max_value))
    return false;

  value = parsed;
  return true;
}

/**
 * Return an environment variable value, or @p default_value if unset or empty.
 */
[[gnu::pure]]
static inline const char *
GetEnvString(const char *env_name, const char *default_value) noexcept
{
  const char *env_value = getenv(env_name);
  return (env_value != nullptr && env_value[0] != '\0')
    ? env_value
    : default_value;
}

/**
 * Parse a boolean environment variable ("1" is true, anything else is false).
 */
[[gnu::pure]]
static inline bool
GetEnvBool(const char *env_name, bool default_value = false) noexcept
{
  const char *env_value = getenv(env_name);
  if (env_value == nullptr)
    return default_value;

  return std::strcmp(env_value, "1") == 0;
}
