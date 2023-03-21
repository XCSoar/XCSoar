// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Error.hpp"
#include "util/Macros.hpp"
#include "Language/Language.hpp"

static const TCHAR *const severity_strings[] = {
  N_("No error"),
  N_("Information"),
  N_("Reduced functionality"),
  N_("Fatal problem"),
};

const TCHAR *
FlarmError::ToString(Severity severity) noexcept
{
  unsigned i = (unsigned)severity;
  return i < ARRAY_SIZE(severity_strings)
    ? severity_strings[i]
    : N_("Unknown");
}

static constexpr struct {
  FlarmError::Code code;
  const TCHAR *string;
} error_strings[] = {
  { FlarmError::Code::FIRMWARE_TIMEOUT, N_("Firmware timeout") },
  { FlarmError::Code::POWER, N_("Power") },
  { FlarmError::Code::GPS_COMMUNICATION, N_("GPS communication") },
  { FlarmError::Code::GPS_CONFIGURATION, N_("GPS configuration") },
  { FlarmError::Code::RF_COMMUNICATION, N_("RF communication") },
  { FlarmError::Code::COMMUNICATION, N_("Communication") },
  { FlarmError::Code::FLASH_MEMORY, N_("Flash memory") },
  { FlarmError::Code::PRESSURE_SENSOR, N_("Pressure sensor") },
  { FlarmError::Code::OBSTACLE_DATABASE, N_("Obstacle database") },
  { FlarmError::Code::FLIGHT_RECORDER, N_("Flight recorder") },
  { FlarmError::Code::TRANSPONDER_RECEIVER,
    N_("Transponder receiver") },
  { FlarmError::Code::OTHER, N_("Other") },
  { FlarmError::Code::OTHER, nullptr }
};

const TCHAR *
FlarmError::ToString(Code code) noexcept
{
  for (auto i = error_strings; i->string != nullptr; ++i)
    if (i->code == code)
      return i->string;

  return N_("Unknown");
}
