// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#import "BluetoothHelper.hpp"
#import "NativeDetectDeviceListener.h"
#import "PortBridge.hpp"
#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>
#include "LogFile.hpp"

@implementation IOSBluetoothManager {
}

- (instancetype)init
{
  self = [super init];
  if (self) {
    _centralManager = [[CBCentralManager alloc] initWithDelegate:self
                                                           queue:nil];
    _discoveredPeripherals = [NSMutableDictionary dictionary];
    _listeners = [NSHashTable weakObjectsHashTable];
  }
  return self;
}

- (void)centralManagerDidUpdateState:(CBCentralManager *)central
{
  switch (central.state) {
    case CBManagerStatePoweredOn:
      LogFormat("Bluetooth is ON");
      break;
    case CBManagerStatePoweredOff:
      LogFormat("Bluetooth is OFF");
      break;
    case CBManagerStateUnsupported:
      LogFormat("Bluetooth unsupported");
      break;
    case CBManagerStateUnauthorized:
      LogFormat("Bluetooth unauthorized");
      break;
    case CBManagerStateResetting:
      LogFormat("Bluetooth resetting");
      break;
    case CBManagerStateUnknown:
    default:
      LogFormat("Bluetooth state unknown");
      break;
  }
}

- (BOOL)isBluetoothEnabled
{
  return self.centralManager.state == CBManagerStatePoweredOn;
}

- (NSString *)nameForDeviceAddress:(NSString *)address
{
  CBPeripheral *peripheral = self.discoveredPeripherals[address];
  if (peripheral && peripheral.name.length > 0) {
    return peripheral.name;
  }
  return nil;
}

- (void)centralManager:(CBCentralManager *)central
    didDiscoverPeripheral:(CBPeripheral *)peripheral
        advertisementData:(NSDictionary<NSString *, id> *)advertisementData
                     RSSI:(NSNumber *)RSSI
{
  NSString *identifier = peripheral.identifier.UUIDString;
  self.discoveredPeripherals[identifier] = peripheral;
  for (NativeDetectDeviceListener *listener in self.listeners) {
    int type = static_cast<int>(DetectDeviceListener::Type::BLUETOOTH_LE);
    [listener onDeviceDetected:type
                       address:identifier
                          name:[self nameForDeviceAddress:identifier]
                      features:0];
  }
}

- (void)startScan
{
  [self.centralManager scanForPeripheralsWithServices:nil options:nil];
}

- (void)stopScan
{
  [self.centralManager stopScan];
}

- (void)addListener:(NativeDetectDeviceListener *)listener
{
  [self startScan];
  if (!listener) return;
  [self.listeners addObject:listener];
}

- (void)removeListener:(NativeDetectDeviceListener *)listener
{
  [self stopScan];
  if (!listener) return;
  [self.listeners removeObject:listener];
}

- (void)connectSensor:(NSString *)deviceAddress listener:(SensorListener &)listener
{
  CBPeripheral *peripheral = self.discoveredPeripherals[deviceAddress];
  if (!peripheral) {
    LogFormat("[iOS] Device %s not found", [deviceAddress UTF8String]);
    return;
  }

  LogFormat("[iOS] Connecting to %s with SensorListener", [peripheral.name UTF8String]);
  peripheral.delegate = self;
  [self.centralManager connectPeripheral:peripheral options:nil];
}

- (PortBridge *)connectToDevice:(NSString *)deviceAddress
{
  CBPeripheral *peripheral = self.discoveredPeripherals[deviceAddress];
  if (!peripheral) {
    LogFormat("Device not found: %s", [deviceAddress UTF8String]);
    return nullptr;
  }

  peripheral.delegate = self;
  [_centralManager connectPeripheral:peripheral options:nil];

  // Do not create the PortBridge here yet, as the connection is asynchronous.
  return nullptr;
}

- (void)centralManager:(CBCentralManager *)central
    didConnectPeripheral:(CBPeripheral *)peripheral
{
  LogFormat("Connected with %s", [peripheral.name UTF8String]);

  PortBridge *bridge = new PortBridge();
  _activeConnections[peripheral] = [NSValue valueWithPointer:bridge];
}

- (void)centralManager:(CBCentralManager *)central
    didFailToConnectPeripheral:(CBPeripheral *)peripheral
                         error:(NSError *)error
{
  LogFormat("Connection to %s failed: %s",
            [peripheral.name UTF8String],
            [error.localizedDescription UTF8String]);
}
@end

BluetoothHelperIOS::BluetoothHelperIOS()
{
  manager = [[IOSBluetoothManager alloc] init];
}

BluetoothHelperIOS::~BluetoothHelperIOS() { manager = nil; }

bool
BluetoothHelperIOS::HasBluetoothSupport() const noexcept
{
  CBManagerState state = manager.centralManager.state;
  if (state == CBManagerStateUnsupported) {
    return false;
  }
  return true;
}

bool
BluetoothHelperIOS::IsEnabled() const noexcept
{
  return [manager isBluetoothEnabled];
}

const char *
BluetoothHelperIOS::GetNameFromAddress(const char *address) const noexcept
{
  if (!address) return nullptr;

  NSString *addrStr = [NSString stringWithUTF8String:address];
  NSString *name = [manager nameForDeviceAddress:addrStr];

  if (!name) return nullptr;

  // TODO: Must return a pointer to static memory
  // Using a simple static buffer here (not thread-safe, for demo purposes)
  static thread_local char buffer[256];
  strncpy(buffer, [name UTF8String], sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';
  return buffer;
}

NativeDetectDeviceListener *
BluetoothHelperIOS::AddDetectDeviceListener(
    DetectDeviceListener &listener) noexcept
{
  NativeDetectDeviceListener *nativeListener =
      [[NativeDetectDeviceListener alloc] initWithCppListener:&listener];
  [manager addListener:nativeListener];
  return nativeListener;
}

void
BluetoothHelperIOS::RemoveDetectDeviceListener(
    NativeDetectDeviceListener *listener) noexcept
{
  [manager removeListener:listener];
}

void
BluetoothHelperIOS::connectSensor(const char *address, SensorListener &listener)
{
	if (!address) return;

	NSString *addrStr = [NSString stringWithUTF8String:address];
	[manager connectSensor:addrStr listener:listener];
}

PortBridge *
BluetoothHelperIOS::connect(const char *address)
{
  if (!address) return nullptr;

  NSString *addrStr = [NSString stringWithUTF8String:address];
  PortBridge *bridge = [manager connectToDevice:addrStr];
  return bridge;
}

PortBridge *
BluetoothHelperIOS::createServer()
{
  // TODO
  return nullptr;
  // PortBridge *bridge = [manager createBluetoothServer];
  // return bridge;
}

extern "C" BluetoothHelper *
CreateBluetoothHelper()
{
  return new BluetoothHelperIOS();
}
