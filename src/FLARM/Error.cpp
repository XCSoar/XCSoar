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
  { FlarmError::Code::FIRMWARE_TIMEOUT, N_("Firmware expired") },
  { FlarmError::Code::FIRMWARE_UPDATE_ERROR, N_("Firmware update error") },
  { FlarmError::Code::POWER, N_("Voltage Low") },
  { FlarmError::Code::UI, N_("UI error") },
  { FlarmError::Code::AUDIO, N_("Audio error") },
  { FlarmError::Code::ADC, N_("ADC error") },
  { FlarmError::Code::SDCARD, N_("SDCARD error") },
  { FlarmError::Code::LED, N_("LED error") },
  { FlarmError::Code::EEPROM, N_("EEPROM error") },
  { FlarmError::Code::GPIO, N_("GPIO error") },
  { FlarmError::Code::GENERAL, N_("General hardware error") },
  { FlarmError::Code::TRANSPONDER_ADSB, N_("Transponder receiver Mode-C/S/ADS-B unserviceable") },
  { FlarmError::Code::GPS_COMMUNICATION, N_("GPS communication") },
  { FlarmError::Code::GPS_CONFIGURATION, N_("Configuration of GPS module") },
  { FlarmError::Code::GPS_ANTENNA, N_("GPS antenna") },
  { FlarmError::Code::RF_COMMUNICATION, N_("RF communication") },
  { FlarmError::Code::ID_SAME, N_("Another FLARM device with the same radio ID is being received") },
  { FlarmError::Code::ID_WRONG, N_("Wrong ICAO 24-bit address or radio ID") },
  { FlarmError::Code::COMMUNICATION, N_("Communication") },
  { FlarmError::Code::FLASH_MEMORY, N_("Flash memory") },
  { FlarmError::Code::PRESSURE_SENSOR, N_("Pressure sensor") },
  { FlarmError::Code::OBSTACLE_DATABASE, N_("Obstacle database expired") },
  { FlarmError::Code::FLIGHT_RECORDER, N_("Flight recorder") },
  { FlarmError::Code::ENL, N_("Engine-noise recording not possible") },
  { FlarmError::Code::INVALID_OBSTACLE_LICENSE, N_("Invalid obstacle database license") },
  { FlarmError::Code::INVALID_IGC_LICENSE, N_("Invalid IGC feature license") },
  { FlarmError::Code::INVALID_AUD_LICENSE, N_("Invalid AUD feature license") },
  { FlarmError::Code::INVALID_ENL_LICENSE, N_("Invalid ENL feature license") },
  { FlarmError::Code::INVALID_RFB_LICENSE, N_("Invalid RFB feature license") },
  { FlarmError::Code::INVALID_TIS_LICENSE, N_("Invalid TIS feature license") },
  { FlarmError::Code::GENERIC, N_("Generic error") },
  { FlarmError::Code::FLASH_FS, N_("Flash File System error") },
  { FlarmError::Code::FAILURE_UPDATING_DISPLAY, N_("Failure updating firmware of external display") },
  { FlarmError::Code::DEVICE_OUTSIDE_REGION, N_("Device is operated outside the designated region. The device does not work.") },
  { FlarmError::Code::RANGE_ANALYZER, N_("Range analyzer") },
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
