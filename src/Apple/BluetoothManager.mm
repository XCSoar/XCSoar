// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#import "BluetoothManager.hpp"
#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

@implementation IOSBluetoothManager

- (instancetype)init
{
  self = [super init];
  if (self) {
    centralManager = [[CBCentralManager alloc] initWithDelegate:self
                                                          queue:nil];
  }
  return self;
}

- (void)centralManagerDidUpdateState:(CBCentralManager *)central
{
  NSLog(@"Bluetooth state updated");
}
@end

BluetoothManagerIOS::BluetoothManagerIOS()
{
  manager = [[IOSBluetoothManager alloc] init];
}

BluetoothManagerIOS::~BluetoothManagerIOS() { manager = nil; }

void
BluetoothManagerIOS::initialize()
{
  NSLog(@"Initialize called");
}

void
BluetoothManagerIOS::scanForDevices()
{
  NSLog(@"Scanning for devices");
}

void
BluetoothManagerIOS::connectToDevice(const char *identifier)
{
  NSLog(@"Connecting to device: %s", identifier);
}

void
BluetoothManagerIOS::sendData(const char *data)
{
  NSLog(@"Sending data: %s", data);
}

extern "C" BluetoothManager *
CreateBluetoothManager()
{
  return new BluetoothManagerIOS();
}
