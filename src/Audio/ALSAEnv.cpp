// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project


#include "ALSAEnv.hpp"

#include "LogFile.hpp"
#include "util/NumberParser.hpp"

#include <cassert>
#include <stdlib.h>


namespace ALSAEnv
{


static constexpr char ALSA_DEVICE_ENV[] = "ALSA_DEVICE";
static constexpr char ALSA_LATENCY_ENV[] = "ALSA_LATENCY";

static constexpr char DEFAULT_ALSA_DEVICE[] = "default";
static constexpr unsigned DEFAULT_ALSA_LATENCY = 100000;


static const char *InitALSADeviceName()
{
  const char *alsa_device = getenv(ALSA_DEVICE_ENV);
  if ((nullptr == alsa_device) || ('\0' == *alsa_device))
    alsa_device = DEFAULT_ALSA_DEVICE;
  LogFormat("Using ALSA PCM device \"%s\" (use environment variable "
                "%s to override)",
            alsa_device, ALSA_DEVICE_ENV);
  return alsa_device;
}

static unsigned InitALSALatency()
{
  unsigned latency;
  const char *latency_env_value = getenv(ALSA_LATENCY_ENV);
  if ((nullptr == latency_env_value) || ('\0' == *latency_env_value)) {
    latency = DEFAULT_ALSA_LATENCY;
  } else {
    char *p;
    latency = ParseUnsigned(latency_env_value, &p);
    if (*p != '\0') {
      LogFormat("Invalid %s value \"%s\"", ALSA_LATENCY_ENV, latency_env_value);
      return false;
    }
  }
  LogFormat("Using ALSA PCM latency %u μs (use environment variable "
                "%s to override)", latency, ALSA_LATENCY_ENV);
  return latency;
}

const char *GetALSADeviceName()
{
  static const char *alsa_device = InitALSADeviceName();
  assert(nullptr != alsa_device);
  return alsa_device;
}

unsigned GetALSALatency()
{
  static unsigned alsa_latency = InitALSALatency();
  return alsa_latency;
}

}
