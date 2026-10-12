// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Device/Driver/BlueFlyVario.hpp"
#include "Internal.hpp"
#include "Device/Port/Port.hpp"

void
BlueFlyDevice::RequestHighConnectionPriority() noexcept
{
  if (high_priority_settled.load())
    return;

  /* Still connecting.  The next sample, or EnableNMEA(), asks again. */
  if (port.GetState() != PortState::READY)
    return;

  port.SetConnectionPriority(Port::ConnectionPriority::HIGH);
  high_priority_settled.store(true);
}

void
BlueFlyDevice::LinkTimeout()
{
  kalman_filter.Reset();
  high_priority_settled.store(false);
}

bool
BlueFlyDevice::EnableNMEA([[maybe_unused]] OperationEnvironment &env)
{
  RequestHighConnectionPriority();
  return true;
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
