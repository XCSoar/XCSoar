// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

@interface IOSBluetoothManager
    : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate>
@property(nonatomic, strong) CBCentralManager *centralManager;
@property (nonatomic, strong) NSMutableDictionary<NSString *, CBPeripheral *> *discoveredPeripherals;
@property (nonatomic, strong) NSMutableSet<NSValue *> *listeners;
@property (nonatomic, strong) NSMutableDictionary<CBPeripheral *, NSValue *> *activeConnections;
@end
