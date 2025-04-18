//
// Created by bruno on 5/12/17.
//

#include "AndroidUsbSerialPort.hpp"
#include "AndroidPort.hpp"
#include "Android/UsbSerialHelper.hpp"
#include "java/Global.hxx"

#include <cassert>

std::unique_ptr<Port>
OpenAndroidUsbSerialPort(UsbSerialHelper &usb_serial_helper,
                         const char *name, unsigned baud,
                         PortListener *listener, DataHandler &handler)
{
  assert(name != nullptr);

  auto *bridge = usb_serial_helper.Connect(Java::GetEnv(), name, baud);
  assert(bridge != nullptr);
  return std::make_unique<AndroidPort>(listener, handler, bridge);
}
