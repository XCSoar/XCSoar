// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

class DetectDeviceListener;
class PortBridge;

/**
 * The CoreBluetooth backend of the iOS #BluetoothHelper.  It owns the
 * #CBCentralManager, performs device discovery and dispatches
 * CoreBluetooth delegate callbacks to the affected #PortBridge
 * instances.
 *
 * All CoreBluetooth work runs on a private serial dispatch queue
 * (#queue).  The public methods may be called from any thread, but
 * must not be called from the dispatch queue itself.
 */
@interface IOSBluetoothManager
    : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate>

/** the serial dispatch queue which runs all CoreBluetooth work */
@property(nonatomic, readonly) dispatch_queue_t queue;

/**
 * Is Bluetooth currently switched on?
 */
- (bool)isEnabled;

/**
 * Register a device detection listener and start scanning for
 * peripherals.  Already discovered peripherals are reported to the
 * new listener immediately.
 */
- (void)addDetectDeviceListener:(DetectDeviceListener *)listener;

/**
 * Unregister a device detection listener.  Scanning is stopped when
 * the last listener has been removed.
 */
- (void)removeDetectDeviceListener:(DetectDeviceListener *)listener;

/**
 * Create a #PortBridge for the peripheral with the given identifier
 * and start connecting to it asynchronously.  This method never
 * returns nullptr; connection problems are reported through the
 * bridge's #PortState.
 *
 * The returned object is owned by the caller; deleting it closes the
 * connection.
 */
- (PortBridge *)connect:(NSString *)address;

/**
 * Unregister a bridge and disconnect from its peripheral.  Invoked by
 * the #PortBridge destructor; after this method returns, no callback
 * will reach the bridge anymore.
 */
- (void)closeBridge:(PortBridge *)bridge;

/**
 * Schedule a PortBridge::SendPendingChunks() call for the given
 * address on the dispatch queue.
 */
- (void)scheduleWrite:(NSString *)address;

@end
