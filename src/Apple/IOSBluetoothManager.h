// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#import "NativeDetectDeviceListener.h"
#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

@interface IOSBluetoothManager
    : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate>

@property(nonatomic, strong, nonnull) CBCentralManager *centralManager;
@property(nonatomic, strong, nonnull)
    NSMutableDictionary<NSString *, CBPeripheral *> *discoveredPeripherals;
@property(nonatomic, nonnull)
    NSHashTable<NativeDetectDeviceListener *> *listeners;
@property(nonatomic, strong, nonnull)
    NSMutableDictionary<CBPeripheral *, NSValue *> *activeConnections;
@property(nonatomic, strong, nullable) NSString *pendingConnectionAddress;
@end
