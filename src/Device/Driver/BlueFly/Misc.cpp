// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/BlueFlyVario.hpp"
#include "Internal.hpp"

void
BlueFlyDevice::LinkTimeout()
{
  kalman_filter.Reset();
}

BlueFlyDevice::BlueFlyDevice(Port &_port)
    :port(_port),
     settings({0}),
     settings_keys(nullptr)
{
  kalman_filter.SetAccelerationVariance(0.3);
}

BlueFlyDevice::~BlueFlyDevice()
{
  free(settings_keys);
}
