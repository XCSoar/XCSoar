//
// Created by bruno on 5/12/17.
//

#include "AndroidUsbSerialPort.hpp"
#include "AndroidPort.hpp"
#include "Android/PortBridge.hpp"
#include "Android/UsbSerialHelper.hpp"
#include "java/Global.hxx"

#include <cassert>

std::unique_ptr<Port>
OpenAndroidUsbSerialPort(const char *name, unsigned baud,
                         PortListener *listener, DataHandler &handler)
{
  assert(name != nullptr);

  JNIEnv *env = Java::GetEnv();
  if (env == nullptr || !UsbSerialHelper::isEnabled(env))
    return nullptr;

  PortBridge *bridge = UsbSerialHelper::connectDevice(env, name, baud);
  if (bridge == nullptr)
    return nullptr;

  return std::make_unique<AndroidPort>(listener, handler, bridge);
}
