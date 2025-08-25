// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#import "BluetoothHelper.hpp"
#import "BluetoothUuids.hpp"
#include "LogFile.hpp"
#import "NativeDetectDeviceListener.h"
#import "PortBridge.hpp"
#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

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
    _activeConnections = [NSMutableDictionary dictionary];
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
  bool peripheralWillBeDetected = true; // TODO

  NSString *identifier = peripheral.identifier.UUIDString;
  self.discoveredPeripherals[identifier] = peripheral;

  // // Falls Pending-Connect → sofort verbinden
  // if (self.pendingConnectionAddress &&
  // 	[identifier isEqualToString:self.pendingConnectionAddress])
  // {
  // 	LogFormat("Pending device %s found, connecting...", [peripheral.name
  // UTF8String]); 	self.pendingConnectionAddress = nil; 	[self.centralManager
  // stopScan];

  // 	PortBridge *bridge = new PortBridge();
  // 	_activeConnections[peripheral] = [NSValue valueWithPointer:bridge];
  // 	peripheral.delegate = self;
  // 	[self.centralManager connectPeripheral:peripheral options:nil];
  // 	return;
  // }

  NSArray<CBUUID *> *serviceUUIDs =
      advertisementData[CBAdvertisementDataServiceUUIDsKey];
  if (serviceUUIDs) {
    auto allServiceUuids = BluetoothUuids::getAllServiceUuids();
    for (CBUUID *uuid in serviceUUIDs) {
      NSString *uuidString = uuid.UUIDString;
      for (auto uuid_sv : allServiceUuids) {
        NSString *serviceUuidString =
            [NSString stringWithUTF8String:uuid_sv.data()];
        if ([serviceUuidString caseInsensitiveCompare:uuidString] ==
            NSOrderedSame) {
          peripheralWillBeDetected = true;
          LogFormat("===> DEBUG Service UUID found in advertisement: %s",
                    uuidString.UTF8String);
        }
      }
    }
  }

  if (peripheralWillBeDetected) {
    uint64_t features = 0;

    for (CBUUID *uuid in serviceUUIDs) {
      NSString *uuidString = uuid.UUIDString;

      NSString *hm10NSString =
          [NSString stringWithUTF8String:BluetoothUuids::HM10_SERVICE.data()];
      NSString *heartRateNSString = [NSString
          stringWithUTF8String:BluetoothUuids::HEART_RATE_SERVICE.data()];
      NSString *flytecNSString = [NSString
          stringWithUTF8String:BluetoothUuids::FLYTEC_SENSBOX_SERVICE.data()];

      if ([uuidString caseInsensitiveCompare:hm10NSString] == NSOrderedSame) {
        features |= DetectDeviceListener::FEATURE_HM10;
      } else if ([uuidString caseInsensitiveCompare:heartRateNSString] ==
                 NSOrderedSame) {
        features |= DetectDeviceListener::FEATURE_HEART_RATE;
      } else if ([uuidString caseInsensitiveCompare:flytecNSString] ==
                 NSOrderedSame) {
        features |= DetectDeviceListener::FEATURE_FLYTEC_SENSBOX;
      }
    }

    // LogFormat("=====> DEBUG FEATURES %llu", features);

    for (NativeDetectDeviceListener *listener in self.listeners) {
      // All devices detected via CoreBluetooth are iOS BLE devices. However,
      // BLUETOOTH_CLASSIC is used here because XCSoar requires it for the
      // interface and driver selection.
      int type =
          static_cast<int>(DetectDeviceListener::Type::BLUETOOTH_CLASSIC);
      [listener onDeviceDetected:type
                         address:identifier
                            name:[self nameForDeviceAddress:identifier]
                        features:features];
    }
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

- (void)connectSensor:(NSString *)deviceAddress
             listener:(SensorListener &)listener
{
  CBPeripheral *peripheral = self.discoveredPeripherals[deviceAddress];
  if (!peripheral) {
    LogFormat("Device %s not found", [deviceAddress UTF8String]);
    return;
  }

  LogFormat("Connecting to %s", [peripheral.name UTF8String]);
  peripheral.delegate = self;
  [self.centralManager connectPeripheral:peripheral options:nil];
}

- (PortBridge *)connectToDevice:(NSString *)deviceAddress
{
  CBPeripheral *peripheral = self.discoveredPeripherals[deviceAddress];

  if (!peripheral) {
    NSUUID *uuid = [[NSUUID alloc] initWithUUIDString:deviceAddress];
    NSArray *peripherals =
        [self.centralManager retrievePeripheralsWithIdentifiers:@[ uuid ]];
    if (peripherals.count > 0) {
      peripheral = peripherals.firstObject;
      self.discoveredPeripherals[deviceAddress] = peripheral;
    }
  }

  if (!peripheral) {
    LogFormat("Device %s not found, scanning...", [deviceAddress UTF8String]);
    self.pendingConnectionAddress = deviceAddress;
    [self.centralManager scanForPeripheralsWithServices:nil options:nil];
    return nullptr; // Erst verbinden, wenn gefunden
  }

  PortBridge *bridge = new PortBridge();
  _activeConnections[peripheral] = [NSValue valueWithPointer:bridge];
  peripheral.delegate = self;

  // Do not create the PortBridge here yet, as the connection is asynchronous.
  [self.centralManager connectPeripheral:peripheral options:nil];
  return bridge;
}

- (void)centralManager:(CBCentralManager *)central
    didConnectPeripheral:(CBPeripheral *)peripheral
{
  LogFormat("Connected with %s", [peripheral.name UTF8String]);
  [peripheral discoverServices:nil];
  //   PortBridge *bridge = new PortBridge();
  //   _activeConnections[peripheral] = [NSValue valueWithPointer:bridge];
}

- (void)peripheral:(CBPeripheral *)peripheral
    didDiscoverServices:(NSError *)error
{
  if (error) {
    LogFormat("Error discovering services: %s", [[error localizedDescription] UTF8String]);
    return;
  }

  for (CBService *service in peripheral.services) {
    [peripheral discoverCharacteristics:nil forService:service];
  }
}

- (void)peripheral:(CBPeripheral *)peripheral
    didDiscoverCharacteristicsForService:(CBService *)service
                                   error:(NSError *)error
{
  if (error) {
    LogFormat("Error discovering characteristics: %s", [[error localizedDescription] UTF8String]);
    return;
  }

  for (CBCharacteristic *characteristic in service.characteristics) {
    if (characteristic.properties & CBCharacteristicPropertyNotify ||
        characteristic.properties & CBCharacteristicPropertyIndicate) {
      [peripheral setNotifyValue:YES forCharacteristic:characteristic];
      [peripheral readValueForCharacteristic:characteristic]; // TODO remove?
    }
  }
}

- (void)peripheral:(CBPeripheral *)peripheral
    didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
                              error:(NSError *)error
{
  if (error != nil) {
    LogFormat("Error updating value for characteristic: %s", [error.localizedDescription UTF8String]);
    return;
  }

  NSValue *bridgeValue = _activeConnections[peripheral];
  if (bridgeValue == nil) {
    LogFormat("No active bridge for peripheral %s", [peripheral.name UTF8String]);
    return;
  }

  PortBridge *bridge = (PortBridge *)[bridgeValue pointerValue];
  NSData *value = characteristic.value;
  if (value == nil || value.length == 0) {
    return;
  }

  const void *bytes = [value bytes];
  size_t length = [value length];

  bridge->getInputListener()->DataReceived({(const std::byte *)bytes, length});
  LogFormat("===> [DEBUG] bridge->getInputListener().DataReceived");
}

- (void)centralManager:(CBCentralManager *)central
    didFailToConnectPeripheral:(CBPeripheral *)peripheral
                         error:(NSError *)error
{
  LogFormat("Connection to %s failed: %s", [peripheral.name UTF8String],
            [error.localizedDescription UTF8String]);
}

- (BOOL)writeData:(NSData *)data
{
  return NO;
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
BluetoothHelperIOS::connectSensor(const char *address,
                                  SensorListener &listener)
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
  return [manager connectToDevice:addrStr];
}

PortBridge *
BluetoothHelperIOS::connectHM10(const char *address)
{
  if (!address) return nullptr;

  NSString *addrStr = [NSString stringWithUTF8String:address];
  return [manager connectToDevice:addrStr];
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
