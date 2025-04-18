// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AndroidIOIOUartPort.hpp"
#include "AndroidPort.hpp"
#include "Android/IOIOHelper.hpp"
#include "java/Global.hxx"

#include <cassert>

std::unique_ptr<Port>
OpenAndroidIOIOUartPort(IOIOHelper &ioio_helper,
                        unsigned uart_id, unsigned baud_rate,
                        PortListener *listener, DataHandler &handler)
{
  assert(uart_id < AndroidIOIOUartPort::getNumberUarts());

  PortBridge *bridge = ioio_helper.openUart(Java::GetEnv(),
                                            uart_id, baud_rate);
  if (bridge == nullptr)
    return nullptr;

  return std::make_unique<AndroidPort>(listener, handler, bridge);
}
