// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BluetoothHelper.hpp"
#include "BluetoothUuids.hpp"
#include "DetectDeviceListener.hpp"
#include "IOSBluetoothManager.h"
#include "PortBridge.hpp"
#include "LogFile.hpp"
#include "thread/Mutex.hxx"

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

#include <cassert>
#include <map>
#include <set>
#include <string>

/**
 * A name cache for BluetoothHelper::GetNameFromAddress(), which must
 * return a pointer to a string which is never freed; same pattern as
 * the Android implementation.  Entries are only ever added, never
 * modified or removed, so the returned pointers stay valid (std::map
 * nodes do not move).
 *
 * Filled by #IOSBluetoothManager on its dispatch queue, read from
 * any thread; protected by #address_to_name_mutex.
 */
static std::map<std::string, std::string, std::less<>> address_to_name;
static Mutex address_to_name_mutex;

/**
 * Remember the name of a peripheral, unless it is unknown or
 * already cached.
 */
static void
CacheName(NSString *address, NSString *name) noexcept
{
  if (name.length == 0)
    return;

  const std::lock_guard lock{address_to_name_mutex};
  address_to_name.emplace(address.UTF8String, name.UTF8String);
}

@implementation IOSBluetoothManager {
  CBCentralManager *centralManager;

  /** all peripherals seen so far, keyed by identifier UUID string */
  NSMutableDictionary<NSString *, CBPeripheral *> *discoveredPeripherals;

  /** the feature mask most recently determined for each peripheral */
  NSMutableDictionary<NSString *, NSNumber *> *peripheralFeatures;

  /** registered detection listeners; only accessed on the queue */
  std::set<DetectDeviceListener *> listeners;

  /**
   * All open bridges, keyed by identifier UUID string.  The bridges
   * are owned by their #ApplePort; they unregister themselves via
   * closeBridge:.
   */
  std::map<std::string, PortBridge *, std::less<>> bridges;
}

- (instancetype)init
{
  self = [super init];
  if (self) {
    _queue = dispatch_queue_create("XCSoar Bluetooth",
                                   DISPATCH_QUEUE_SERIAL);
    discoveredPeripherals = [NSMutableDictionary dictionary];
    peripheralFeatures = [NSMutableDictionary dictionary];
    centralManager = [[CBCentralManager alloc] initWithDelegate:self
                                                          queue:_queue];
  }
  return self;
}

- (bool)isEnabled
{
  return centralManager.state == CBManagerStatePoweredOn;
}

/**
 * Identify the advertised service UUIDs and convert them to a
 * feature flag bit set.
 *
 * Keep this in sync with getFeatures() in
 * android/src/BluetoothHelper.java.
 */
static uint64_t
FeaturesFromAdvertisedServices(NSArray<CBUUID *> *serviceUuids) noexcept
{
  using namespace BluetoothUuids;

  uint64_t features = 0;

  for (CBUUID *uuid in serviceUuids) {
    if ([uuid isEqual:Uuid<HM10_SERVICE>()] ||
        [uuid isEqual:Uuid<NORDIC_UART_SERVICE>()] ||
        [uuid isEqual:Uuid<ISSC_UART_SERVICE>()])
      /* any BLE UART service can be used with the generic "BLE
         serial" port implementation */
      features |= DetectDeviceListener::FEATURE_BLE_SERIAL;
    else if ([uuid isEqual:Uuid<HEART_RATE_SERVICE>()])
      features |= DetectDeviceListener::FEATURE_HEART_RATE;
    else if ([uuid isEqual:Uuid<FLYTEC_SENSBOX_SERVICE>()])
      features |= DetectDeviceListener::FEATURE_FLYTEC_SENSBOX;
  }

  return features;
}

/**
 * Look up the bridge for the given peripheral.  Must be called on the
 * dispatch queue.
 *
 * @return the bridge or nullptr if the peripheral has no open bridge
 */
- (PortBridge *)bridgeForPeripheral:(CBPeripheral *)peripheral
{
  auto i = bridges.find(std::string_view{
      peripheral.identifier.UUIDString.UTF8String});
  return i != bridges.end() ? i->second : nullptr;
}

/**
 * Start or stop scanning depending on whether somebody is currently
 * interested in scan results.  Must be called on the dispatch queue.
 */
- (void)updateScanState
{
  if (centralManager.state != CBManagerStatePoweredOn)
    return;

  bool want_scan = !listeners.empty();

  /* keep scanning while a configured device has not been discovered
     yet */
  for (const auto &i : bridges)
    if (i.second->GetPeripheral() == nil)
      want_scan = true;

  if (want_scan && !centralManager.isScanning)
    /* unlike Android, scan without a service UUID filter: many BLE
       UART bridges do not advertise their serial service UUID, and
       iOS applications cannot enumerate bonded devices; note that
       unfiltered scans yield no results while the app is in the
       background (the "bluetooth-central" background mode only
       permits filtered scans there), which is fine because scanning
       is a foreground activity (port picker), and pending
       connectPeripheral requests are not affected */
    [centralManager scanForPeripheralsWithServices:nil options:nil];
  else if (!want_scan && centralManager.isScanning)
    [centralManager stopScan];
}

/**
 * Attach a peripheral to the given bridge and start connecting.  If
 * the peripheral is not yet known, a scan is started instead.  Must
 * be called on the dispatch queue.
 */
- (void)attachAndConnectBridge:(PortBridge *)bridge
{
  if (centralManager.state != CBManagerStatePoweredOn) {
    /* postponed until centralManagerDidUpdateState reports that
       Bluetooth is available */
    [self updateScanState];
    return;
  }

  NSString *address = bridge->GetAddress();
  CBPeripheral *peripheral = discoveredPeripherals[address];

  if (peripheral == nil) {
    /* not discovered in this session; ask the system whether it
       already knows the peripheral */
    NSUUID *uuid = [[NSUUID alloc] initWithUUIDString:address];
    if (uuid != nil) {
      NSArray<CBPeripheral *> *known =
        [centralManager retrievePeripheralsWithIdentifiers:@[uuid]];
      if (known.count > 0) {
        peripheral = known.firstObject;
        discoveredPeripherals[address] = peripheral;
        CacheName(address, peripheral.name);
      }
    }
  }

  if (peripheral != nil) {
    /* the request has no timeout; it completes whenever the
       peripheral accepts the connection */
    LogFormat("Bluetooth: connecting to %s", address.UTF8String);
    bridge->OnPeripheralAttached(peripheral);
    peripheral.delegate = self;
    [centralManager connectPeripheral:peripheral options:nil];
  } else
    LogFormat("Bluetooth: device %s is not known, scanning for it",
              address.UTF8String);

  [self updateScanState];
}

- (void)addDetectDeviceListener:(DetectDeviceListener *)listener
{
  dispatch_sync(_queue, ^{
    listeners.insert(listener);

    /* similar to Android's bonded device enumeration: register
       peripherals which are already connected at the system level and
       provide a known UART service */
    if (centralManager.state == CBManagerStatePoweredOn) {
      NSArray<CBPeripheral *> *connected =
        [centralManager retrieveConnectedPeripheralsWithServices:
                          BluetoothUuids::SerialServiceUuids()];
      for (CBPeripheral *peripheral in connected) {
        NSString *address = peripheral.identifier.UUIDString;
        discoveredPeripherals[address] = peripheral;
        CacheName(address, peripheral.name);
        if (peripheralFeatures[address] == nil)
          peripheralFeatures[address] =
            @(DetectDeviceListener::FEATURE_BLE_SERIAL);
      }
    }

    /* report all peripherals discovered so far to the new listener */
    for (NSString *address in discoveredPeripherals) {
      CBPeripheral *peripheral = discoveredPeripherals[address];
      NSString *name = peripheral.name;
      if (name.length == 0)
        continue;

      /* like in the discovery callback, every peripheral is a
         potential serial bridge */
      const uint64_t features =
        peripheralFeatures[address].unsignedLongLongValue |
        DetectDeviceListener::FEATURE_BLE_SERIAL;

      listener->OnDeviceDetected(DetectDeviceListener::Type::BLUETOOTH_LE,
                                 address.UTF8String, name.UTF8String,
                                 features);
    }

    [self updateScanState];
  });
}

- (void)removeDetectDeviceListener:(DetectDeviceListener *)listener
{
  dispatch_sync(_queue, ^{
    listeners.erase(listener);

    /* this stops the scan when the last listener is removed (unless
       a bridge is still waiting for its peripheral) */
    [self updateScanState];
  });
}

- (PortBridge *)connect:(NSString *)address
{
  __block PortBridge *bridge = nullptr;

  dispatch_sync(_queue, ^{
    bridge = new PortBridge(self, address);
    bridges[address.UTF8String] = bridge;
    [self attachAndConnectBridge:bridge];
  });

  return bridge;
}

- (void)closeBridge:(PortBridge *)bridge
{
  dispatch_sync(_queue, ^{
    NSString *address = bridge->GetAddress();

    if (auto i = bridges.find(std::string_view{address.UTF8String});
        i != bridges.end() && i->second == bridge)
      bridges.erase(i);

    if (CBPeripheral *peripheral = bridge->GetPeripheral();
        peripheral != nil)
      [centralManager cancelPeripheralConnection:peripheral];

    [self updateScanState];
  });
}

- (void)scheduleWrite:(NSString *)address
{
  dispatch_async(_queue, ^{
    if (auto i = bridges.find(std::string_view{address.UTF8String});
        i != bridges.end())
      i->second->SendPendingChunks();
  });
}

/* CBCentralManagerDelegate */

- (void)centralManagerDidUpdateState:(CBCentralManager *)central
{
  switch (central.state) {
  case CBManagerStatePoweredOn:
    LogFormat("Bluetooth: powered on");

    /* (re)connect all configured devices */
    for (const auto &i : bridges)
      [self attachAndConnectBridge:i.second];

    [self updateScanState];
    break;

  case CBManagerStatePoweredOff:
  case CBManagerStateResetting:
    LogFormat("Bluetooth: powered off");

    /* all CBPeripheral instances are invalid now */
    [discoveredPeripherals removeAllObjects];

    for (const auto &i : bridges) {
      i.second->OnDisconnected();
      i.second->OnPeripheralLost();
    }
    break;

  case CBManagerStateUnauthorized:
    LogFormat("Bluetooth: access not authorized");
    break;

  case CBManagerStateUnsupported:
    LogFormat("Bluetooth: not supported on this device");
    break;

  case CBManagerStateUnknown:
    break;
  }
}

- (void)centralManager:(CBCentralManager *)central
 didDiscoverPeripheral:(CBPeripheral *)peripheral
     advertisementData:(NSDictionary<NSString *, id> *)advertisementData
                  RSSI:(NSNumber *)RSSI
{
  NSString *address = peripheral.identifier.UUIDString;
  discoveredPeripherals[address] = peripheral;

  /* is a configured port waiting for this peripheral? */
  if (auto i = bridges.find(std::string_view{address.UTF8String});
      i != bridges.end() && i->second->GetPeripheral() == nil) {
    LogFormat("Bluetooth: connecting to %s", address.UTF8String);
    i->second->OnPeripheralAttached(peripheral);
    peripheral.delegate = self;
    [centralManager connectPeripheral:peripheral options:nil];
    [self updateScanState];
  }

  uint64_t features = FeaturesFromAdvertisedServices(
      advertisementData[CBAdvertisementDataServiceUUIDsKey]);

  NSString *name = peripheral.name;
  if (name.length == 0)
    name = advertisementData[CBAdvertisementDataLocalNameKey];

  CacheName(address, name);

#ifndef NDEBUG
  /* unlike the port picker, which also lists peripherals known from
     the system cache, this proves that the device is advertising
     right now */
  LogFormat("Bluetooth: discovered %s (%s) rssi=%d",
            address.UTF8String,
            name.length > 0 ? name.UTF8String : "?",
            RSSI.intValue);
#endif

  if (features == 0 && name.length == 0)
    /* skip anonymous peripherals without any recognised service to
       avoid cluttering the port picker with unusable entries */
    return;

  /* consider every peripheral a potential serial bridge: BLE UART
     bridges often do not advertise their serial service UUID at all,
     or advertise a sensor service (e.g. Flytec Sensbox) in addition
     to it; and since BLE sensors are not supported on iOS (yet), a
     "BLE sensor" port would be a dead end anyway - without this
     flag, such a device could not be configured as a port and e.g.
     the FLARM flight download would stay unavailable */
  features |= DetectDeviceListener::FEATURE_BLE_SERIAL;

  peripheralFeatures[address] = @(features);

  for (auto *listener : listeners)
    listener->OnDeviceDetected(DetectDeviceListener::Type::BLUETOOTH_LE,
                               address.UTF8String,
                               name.length > 0 ? name.UTF8String : nullptr,
                               features);
}

- (void)centralManager:(CBCentralManager *)central
  didConnectPeripheral:(CBPeripheral *)peripheral
{
  CacheName(peripheral.identifier.UUIDString, peripheral.name);

  if (auto *bridge = [self bridgeForPeripheral:peripheral])
    bridge->OnConnected();
}

- (void)centralManager:(CBCentralManager *)central
didFailToConnectPeripheral:(CBPeripheral *)peripheral
                 error:(NSError *)error
{
  LogFormat("Bluetooth: connection to %s failed: %s",
            peripheral.identifier.UUIDString.UTF8String,
            error.localizedDescription.UTF8String);

  if ([self bridgeForPeripheral:peripheral] == nullptr)
    return;

  /* retry after a small delay, but only if the bridge still exists
     by then */
  dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC), _queue, ^{
    if ([self bridgeForPeripheral:peripheral] != nullptr &&
        centralManager.state == CBManagerStatePoweredOn)
      [centralManager connectPeripheral:peripheral options:nil];
  });
}

- (void)centralManager:(CBCentralManager *)central
didDisconnectPeripheral:(CBPeripheral *)peripheral
                 error:(NSError *)error
{
  if (error != nil)
    LogFormat("Bluetooth: %s disconnected: %s",
              peripheral.identifier.UUIDString.UTF8String,
              error.localizedDescription.UTF8String);

  if (auto *bridge = [self bridgeForPeripheral:peripheral]) {
    bridge->OnDisconnected();

    /* attempt to reconnect, like Android's HM10Port; the connection
       request does not time out and completes whenever the device
       becomes reachable again */
    if (centralManager.state == CBManagerStatePoweredOn) {
      LogFormat("Bluetooth: reconnecting to %s",
                peripheral.identifier.UUIDString.UTF8String);
      [centralManager connectPeripheral:peripheral options:nil];
    }
  }
}

/* CBPeripheralDelegate */

- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverServices:(NSError *)error
{
  if (auto *bridge = [self bridgeForPeripheral:peripheral])
    bridge->OnServicesDiscovered(error);
}

- (void)peripheral:(CBPeripheral *)peripheral
didDiscoverCharacteristicsForService:(CBService *)service
             error:(NSError *)error
{
  if (auto *bridge = [self bridgeForPeripheral:peripheral])
    bridge->OnCharacteristicsDiscovered(error);
}

- (void)peripheral:(CBPeripheral *)peripheral
didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error
{
  if (error != nil)
    LogFormat("Bluetooth: enabling notifications for %s failed: %s",
              peripheral.identifier.UUIDString.UTF8String,
              error.localizedDescription.UTF8String);
}

- (void)peripheral:(CBPeripheral *)peripheral
didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error
{
  if (error != nil)
    return;

  if (auto *bridge = [self bridgeForPeripheral:peripheral])
    bridge->OnDataReceived(characteristic, characteristic.value);
}

- (void)peripheral:(CBPeripheral *)peripheral
didWriteValueForCharacteristic:(CBCharacteristic *)characteristic
             error:(NSError *)error
{
  if (auto *bridge = [self bridgeForPeripheral:peripheral])
    bridge->OnWriteCompleted(error);
}

- (void)peripheralIsReadyToSendWriteWithoutResponse:(CBPeripheral *)peripheral
{
  if (auto *bridge = [self bridgeForPeripheral:peripheral])
    bridge->OnReadyToSendWriteWithoutResponse();
}

@end

BluetoothHelper::BluetoothHelper() noexcept
  :manager([[IOSBluetoothManager alloc] init])
{
}

BluetoothHelper::~BluetoothHelper() noexcept
{
  manager = nil;
}

bool
BluetoothHelper::IsEnabled() const noexcept
{
  return [manager isEnabled];
}

const char *
BluetoothHelper::GetNameFromAddress(const char *address) const noexcept
{
  assert(address != nullptr);

  const std::lock_guard lock{address_to_name_mutex};

  if (auto i = address_to_name.find(std::string_view{address});
      i != address_to_name.end())
    return i->second.c_str();

  return nullptr;
}

void
BluetoothHelper::AddDetectDeviceListener(DetectDeviceListener &l) noexcept
{
  [manager addDetectDeviceListener:&l];
}

void
BluetoothHelper::RemoveDetectDeviceListener(DetectDeviceListener &l) noexcept
{
  [manager removeDetectDeviceListener:&l];
}

PortBridge *
BluetoothHelper::connect(const char *address) noexcept
{
  assert(address != nullptr);

  return [manager connect:[NSString stringWithUTF8String:address]];
}
