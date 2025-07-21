// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

@interface IOSBluetoothManager
    : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate>
@end
