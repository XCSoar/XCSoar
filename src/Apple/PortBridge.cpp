// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PortBridge.hpp"
#include "NativeInputListener.hpp"
#include "NativePortListener.hpp"
#include "LogFile.hpp"

#include "Apple/BluetoothHelper.hpp"
#include "Apple/Main.hpp"

#include <span>
#include <string.h>


PortBridge::PortBridge() {
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

std::size_t
PortBridge::write(std::span<const std::byte> src)
{
  (void)src;
  // TODO
  return -1;
	if (bluetooth_helper != nullptr) {
		// NSData *data = [NSData dataWithBytes:src.data() length:src.size()];
		// IOSBluetoothManager *manager = [IOSBluetoothManager sharedInstance];
		// // BOOL success = [manager writeData:data];
		// const std::string address = getAddress();  // PortBridge::getAddress()
		// NSString *addrStr = [NSString stringWithUTF8String:address.c_str()];
		// BOOL success = [manager writeData:data toDeviceAddress:addrStr];
		BOOL success = NO;

		if (!success)
			throw std::runtime_error{"Port write failed"};
		LogFormat("=====> PortBridge::write size %zu", src.size());
	
		return src.size(); // oder tatsächliche Anzahl gesendeter Bytes
	}

	return -1;

}
