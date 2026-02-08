// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PortBridge.hpp"
#include "LogFile.hpp"
#include "NativeInputListener.hpp"
#include "NativePortListener.hpp"

#include "Apple/BluetoothHelper.hpp"
#include "Apple/Services.hpp"

#include <span>
#include <string.h>

PortBridge::PortBridge(const char *address): deviceAddress(address)
{
  setListener(new NativePortListener());
  setInputListener(new NativeInputListener());
}

void
PortBridge::setListener(PortListener *listener)
{
  portListener = listener;
}

void
PortBridge::setInputListener(DataHandler *handler)
{
  // TODO
  inputListener = handler;
}

DataHandler *
PortBridge::getInputListener()
{
  return inputListener;
}

std::size_t
PortBridge::write(std::span<const std::byte> src)
{
  if (bluetooth_helper != nullptr) {
    auto *helper = dynamic_cast<BluetoothHelperIOS *>(bluetooth_helper);
    if (!helper) return -1;

    IOSBluetoothManager *manager = helper->getManager();
    NSData *data = [NSData dataWithBytes:src.data() length:src.size()];
    NSString *addrStr = [NSString stringWithUTF8String:deviceAddress.c_str()];
    BOOL success = [manager writeData:data toDeviceAddress:addrStr];

    if (!success) throw std::runtime_error{"Port write failed"};
    // LogFormat("=====> PortBridge::write size %zu", src.size());

    return src.size();
  }

  return -1;
}
