// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#import "BluetoothHelper.hpp"
#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

@implementation IOSBluetoothManager {}

- (instancetype)init {
	self = [super init];
	if (self) {
		_centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
		_discoveredPeripherals = [NSMutableDictionary dictionary];
        _listeners = [NSMutableSet set];
	}
	return self;
}
- (void)centralManagerDidUpdateState:(CBCentralManager *)central {
    // Beispielhafte einfache Implementierung:
    switch (central.state) {
        case CBManagerStatePoweredOn:
            NSLog(@"Bluetooth is ON");
            break;
        case CBManagerStatePoweredOff:
            NSLog(@"Bluetooth is OFF");
            break;
        case CBManagerStateUnsupported:
            NSLog(@"Bluetooth unsupported");
            break;
        case CBManagerStateUnauthorized:
            NSLog(@"Bluetooth unauthorized");
            break;
        case CBManagerStateResetting:
            NSLog(@"Bluetooth resetting");
            break;
        case CBManagerStateUnknown:
        default:
            NSLog(@"Bluetooth state unknown");
            break;
    }
}
- (BOOL)isBluetoothEnabled {
	return self.centralManager.state == CBManagerStatePoweredOn;
}
- (NSString *)nameForDeviceAddress:(NSString *)address {
	CBPeripheral *peripheral = self.discoveredPeripherals[address];
	if (peripheral && peripheral.name.length > 0) {
		return peripheral.name;
	}
	return nil;
}
- (void)centralManager:(CBCentralManager *)central
 didDiscoverPeripheral:(CBPeripheral *)peripheral
     advertisementData:(NSDictionary<NSString *, id> *)advertisementData
                  RSSI:(NSNumber *)RSSI {

	// TODO: Using address as a key does not work directly on iOS!
	// Simulate it using the identifier (UUID) instead.
	NSString *identifier = peripheral.identifier.UUIDString;
	self.discoveredPeripherals[identifier] = peripheral;
    for (id listener in self.listeners) {
        auto* cppListener = (__bridge DetectDeviceListener*)listener;
	    cppListener->OnDeviceDetected(DetectDeviceListener::Type::BLUETOOTH_LE,
	                              identifier.UTF8String,
	                              [self nameForDeviceAddress:identifier].UTF8String,
	                              0);
	}
}
- (void)addListener:(DetectDeviceListener *)listener {
    if (!listener) return;
    [self.listeners addObject:[NSValue valueWithPointer:listener]];
}
- (void)removeListener:(DetectDeviceListener *)listener {
    if (!listener) return;
    [self.listeners removeObject:[NSValue valueWithPointer:listener]];
}
- (PortBridge *)connectToDevice:(NSString *)deviceAddress {
    CBPeripheral *peripheral = self.discoveredPeripherals[deviceAddress];
    if (!peripheral) {
        NSLog(@"Device not found: %@", deviceAddress);
        return nullptr;
    }

    peripheral.delegate = self;
    [_centralManager connectPeripheral:peripheral options:nil];
    
    // Do not create the PortBridge here yet, as the connection is asynchronous.
    return nullptr;
}

- (void)centralManager:(CBCentralManager *)central didConnectPeripheral:(CBPeripheral *)peripheral {
    NSLog(@"Verbunden mit %@", peripheral.name);
    // TODO
    PortBridge *bridge = new PortBridge();
    _activeConnections[peripheral] = [NSValue valueWithPointer:bridge];
}

- (void)centralManager:(CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)peripheral error:(NSError *)error {
    NSLog(@"Verbindung zu %@ fehlgeschlagen: %@", peripheral.name, error.localizedDescription);
}
@end

BluetoothHelperIOS::BluetoothHelperIOS()
{
  manager = [[IOSBluetoothManager alloc] init];
}

BluetoothHelperIOS::~BluetoothHelperIOS() {
    manager = nil;
}

bool BluetoothHelperIOS::HasBluetoothSupport() const noexcept {
    CBManagerState state = manager.centralManager.state;
        if (state == CBManagerStateUnsupported) {
        return false;
    }
    return true;
}

bool BluetoothHelperIOS::IsEnabled() const noexcept {
  return [manager isBluetoothEnabled];
}

const char *BluetoothHelperIOS::GetNameFromAddress(const char *address) const noexcept {
  if (!address)
    return nullptr;

  NSString *addrStr = [NSString stringWithUTF8String:address];
  NSString *name = [manager nameForDeviceAddress:addrStr];

  if (!name)
    return nullptr;

  // TODO: Must return a pointer to static memory
  // Using a simple static buffer here (not thread-safe, for demo purposes)
  static thread_local char buffer[256];
  strncpy(buffer, [name UTF8String], sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';
  return buffer;
}

void BluetoothHelperIOS::AddDetectDeviceListener(DetectDeviceListener &listener) noexcept {
  [manager addListener:&listener];
}

void BluetoothHelperIOS::RemoveDetectDeviceListener(DetectDeviceListener &listener) noexcept {
  [manager removeListener:&listener];
}

PortBridge *BluetoothHelperIOS::connect(const char *address) {
  if (!address)
    return nullptr;

  NSString *addrStr = [NSString stringWithUTF8String:address];
  PortBridge *bridge = [manager connectToDevice:addrStr];
  return bridge;
}

PortBridge *BluetoothHelperIOS::createServer() {
  // TODO
  return nullptr;
  PortBridge *bridge = [manager createBluetoothServer];
  return bridge;
}

extern "C" BluetoothHelper *
CreateBluetoothHelper()
{
  return new BluetoothHelperIOS();
}
