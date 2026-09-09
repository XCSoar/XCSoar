// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PortBridge.hpp"
#include "BluetoothUuids.hpp"
#include "IOSBluetoothManager.h"
#include "LogFile.hpp"
#include "Device/Port/Listener.hpp"
#include "io/DataHandler.hpp"

#include <algorithm>
#include <chrono>

/**
 * How long Write() and Drain() wait for the write buffer; equal to
 * the timeout used by Android's HM10WriteBuffer.
 */
static constexpr auto WRITE_TIMEOUT = std::chrono::seconds(5);

PortBridge::PortBridge(IOSBluetoothManager *_manager,
                       NSString *_address) noexcept
  :manager(_manager), address(_address)
{
}

PortBridge::~PortBridge() noexcept
{
  {
    const std::lock_guard lock{mutex};
    closed = true;
    cond.notify_all();
  }

  /* synchronously unregister from the manager; after this call, no
     CoreBluetooth callback can reach this object anymore */
  [manager closeBridge:this];
}

void
PortBridge::SetListener(PortListener *_listener) noexcept
{
  const std::lock_guard lock{mutex};
  listener = _listener;
}

void
PortBridge::SetInputListener(DataHandler *_handler) noexcept
{
  const std::lock_guard lock{mutex};
  handler = _handler;
}

PortState
PortBridge::GetState() const noexcept
{
  const std::lock_guard lock{mutex};
  return state;
}

void
PortBridge::SetState(PortState new_state) noexcept
{
  PortListener *l;

  {
    const std::lock_guard lock{mutex};
    if (new_state == state)
      return;

    state = new_state;
    l = listener;
    cond.notify_all();
  }

  if (l != nullptr)
    l->PortStateChanged();
}

void
PortBridge::Error(const char *msg) noexcept
{
  PortListener *l;

  {
    const std::lock_guard lock{mutex};
    state = PortState::FAILED;
    l = listener;
    write_buffer.Clear();
    cond.notify_all();
  }

  if (l != nullptr)
    l->PortError(msg);
}

bool
PortBridge::Drain() noexcept
{
  std::unique_lock lock{mutex};

  cond.wait_for(lock, WRITE_TIMEOUT, [this]{
    return write_buffer.empty() || closed || state == PortState::FAILED;
  });

  return write_buffer.empty() && state != PortState::FAILED;
}

std::size_t
PortBridge::Write(std::span<const std::byte> src) noexcept
{
  std::size_t nbytes;

  {
    std::unique_lock lock{mutex};

    /* wait for space in the write buffer, like Android's
       HM10WriteBuffer.drainSome() */
    cond.wait_for(lock, WRITE_TIMEOUT, [this]{
      return !write_buffer.IsFull() || closed ||
        state == PortState::FAILED;
    });

    if (closed || state == PortState::FAILED)
      return 0;

    nbytes = write_buffer.MoveFrom(src);
  }

  if (nbytes > 0)
    /* have the dispatch queue transmit the new data; the manager
       looks up this bridge by its address, which is safe even if it
       gets closed and deleted before the block runs */
    [manager scheduleWrite:address];

  return nbytes;
}

inline bool
PortBridge::CanSendChunk() const noexcept
{
  if (write_type == CBCharacteristicWriteWithResponse)
    return !write_pending;

  if (@available(iOS 11, macOS 10.13, *))
    return peripheral.canSendWriteWithoutResponse;

  /* no flow control available on this OS version; send blindly */
  return true;
}

void
PortBridge::SendPendingChunks() noexcept
{
  if (peripheral == nil || tx_characteristic == nil)
    /* not connected yet; the data stays buffered and is flushed by
       SelectCharacteristics() */
    return;

  while (CanSendChunk()) {
    NSData *chunk;

    {
      const std::lock_guard lock{mutex};

      const auto r = write_buffer.Read();
      if (r.empty())
        return;

      const std::size_t nbytes = std::min(r.size(), chunk_size);
      chunk = [NSData dataWithBytes:r.data() length:nbytes];
      write_buffer.Consume(nbytes);

      /* wake up Write() and Drain() */
      cond.notify_all();
    }

    if (write_type == CBCharacteristicWriteWithResponse)
      write_pending = true;

    [peripheral writeValue:chunk
         forCharacteristic:tx_characteristic
                      type:write_type];
  }
}

void
PortBridge::OnPeripheralAttached(CBPeripheral *_peripheral) noexcept
{
  peripheral = _peripheral;
}

void
PortBridge::OnPeripheralLost() noexcept
{
  peripheral = nil;
}

void
PortBridge::OnConnected() noexcept
{
  LogFormat("Bluetooth: connected to %s", address.UTF8String);

  rx_characteristic = nil;
  tx_characteristic = nil;

  [peripheral discoverServices:nil];
}

void
PortBridge::OnDisconnected() noexcept
{
  rx_characteristic = nil;
  tx_characteristic = nil;
  write_pending = false;
  pending_characteristic_discoveries = 0;

  {
    const std::lock_guard lock{mutex};
    /* discard buffered data; sending the rest of a partial datagram
       after reconnecting would only confuse the peer */
    write_buffer.Clear();
    cond.notify_all();
  }

  SetState(PortState::LIMBO);
}

void
PortBridge::OnServicesDiscovered(NSError *error) noexcept
{
  if (error != nil) {
    LogFormat("Bluetooth: service discovery for %s failed: %s",
              address.UTF8String, error.localizedDescription.UTF8String);
    Error("Bluetooth service discovery failed");
    return;
  }

  if (peripheral.services.count == 0) {
    Error("Bluetooth device has no services");
    return;
  }

  pending_characteristic_discoveries = peripheral.services.count;
  for (CBService *service in peripheral.services)
    [peripheral discoverCharacteristics:nil forService:service];
}

void
PortBridge::OnCharacteristicsDiscovered(NSError *error) noexcept
{
  if (error != nil)
    LogFormat("Bluetooth: characteristic discovery for %s failed: %s",
              address.UTF8String, error.localizedDescription.UTF8String);

  if (pending_characteristic_discoveries == 0)
    return;

  if (--pending_characteristic_discoveries == 0)
    SelectCharacteristics();
}

[[gnu::pure]]
static bool
CanNotify(CBCharacteristic *c) noexcept
{
  return (c.properties & (CBCharacteristicPropertyNotify |
                          CBCharacteristicPropertyIndicate)) != 0;
}

[[gnu::pure]]
static bool
CanWrite(CBCharacteristic *c) noexcept
{
  return (c.properties & (CBCharacteristicPropertyWrite |
                          CBCharacteristicPropertyWriteWithoutResponse)) != 0;
}

/**
 * Find a characteristic with the given UUID.
 */
[[gnu::pure]]
static CBCharacteristic *
FindCharacteristic(CBService *service, CBUUID *uuid) noexcept
{
  for (CBCharacteristic *c in service.characteristics)
    if ([c.UUID isEqual:uuid])
      return c;

  return nil;
}

/**
 * Find the first characteristic in this service which supports
 * writing.
 */
[[gnu::pure]]
static CBCharacteristic *
FindWritableCharacteristic(CBService *service) noexcept
{
  for (CBCharacteristic *c in service.characteristics)
    if (CanWrite(c))
      return c;

  return nil;
}

void
PortBridge::SelectCharacteristics() noexcept
{
  using namespace BluetoothUuids;

  CBCharacteristic *rx = nil, *tx = nil;

#ifndef NDEBUG
  /* list what the peripheral offers; without this, a device which
     serves its data on a service we do not know about looks like a
     device which does not work at all */
  for (CBService *service in peripheral.services)
    for (CBCharacteristic *c in service.characteristics)
      LogFormat("Bluetooth: %s has service %s characteristic %s props=0x%x",
                address.UTF8String,
                service.UUID.UUIDString.UTF8String,
                c.UUID.UUIDString.UTF8String,
                unsigned(c.properties));
#endif

  for (CBService *service in peripheral.services) {
    if ([service.UUID isEqual:Uuid<HM10_SERVICE>()]) {
      /* first choice: HM-10, which uses a single characteristic for
         both directions; some clones however make it notify-only and
         provide a separate characteristic (e.g. FFE2) for writing */
      if (CBCharacteristic *c =
            FindCharacteristic(service, Uuid<HM10_RX_TX_CHARACTERISTIC>());
          c != nil && CanNotify(c)) {
        if (CBCharacteristic *w =
              CanWrite(c) ? c : FindWritableCharacteristic(service);
            w != nil) {
          rx = c;
          tx = w;
          break;
        }
      }
    } else if ([service.UUID isEqual:Uuid<NORDIC_UART_SERVICE>()]) {
      /* second choice: the Nordic UART Service */
      CBCharacteristic *nus_rx =
        FindCharacteristic(service, Uuid<NORDIC_UART_TX_CHARACTERISTIC>());
      CBCharacteristic *nus_tx =
        FindCharacteristic(service, Uuid<NORDIC_UART_RX_CHARACTERISTIC>());
      if (nus_rx != nil && CanNotify(nus_rx) &&
          nus_tx != nil && CanWrite(nus_tx)) {
        rx = nus_rx;
        tx = nus_tx;
      }
    } else if ([service.UUID isEqual:Uuid<ISSC_UART_SERVICE>()]) {
      /* third choice: the Microchip/ISSC transparent UART */
      CBCharacteristic *issc_rx =
        FindCharacteristic(service, Uuid<ISSC_UART_TX_CHARACTERISTIC>());
      CBCharacteristic *issc_tx =
        FindCharacteristic(service, Uuid<ISSC_UART_RX_CHARACTERISTIC>());
      if (rx == nil && issc_rx != nil && CanNotify(issc_rx) &&
          issc_tx != nil && CanWrite(issc_tx)) {
        rx = issc_rx;
        tx = issc_tx;
      }
    }
  }

  if (rx == nil || tx == nil) {
    /* fallback for UART bridges with proprietary UUIDs and for
       non-conforming implementations of the services above: use the
       first service which contains both a notifying and a writable
       characteristic */
    for (CBService *service in peripheral.services) {
      CBCharacteristic *service_rx = nil, *service_tx = nil;

      for (CBCharacteristic *c in service.characteristics) {
        if (service_rx == nil && CanNotify(c))
          service_rx = c;
        if (service_tx == nil && CanWrite(c))
          service_tx = c;
      }

      if (service_rx != nil && service_tx != nil) {
        rx = service_rx;
        tx = service_tx;
        break;
      }
    }
  }

  if (rx == nil || tx == nil) {
    Error("No usable Bluetooth characteristics found");
    return;
  }

  rx_characteristic = rx;
  tx_characteristic = tx;

  write_type = (tx.properties & CBCharacteristicPropertyWriteWithoutResponse)
    ? CBCharacteristicWriteWithoutResponse
    : CBCharacteristicWriteWithResponse;

  /* iOS negotiates the ATT MTU automatically right after
     connecting; unlike Android (BluetoothGatt.requestMtu()), an
     application cannot request a larger one.
     maximumWriteValueLengthForType: reflects the negotiated result:
     for writes without response it is the MTU minus the 3 byte ATT
     header.  If a peripheral stays at the small default, that limit
     is imposed by the bridge firmware (often configurable there,
     e.g. "AT+MTU" on HM-10 clones). */
  const std::size_t max_wor = [peripheral
    maximumWriteValueLengthForType:CBCharacteristicWriteWithoutResponse];
  const std::size_t max_wr = [peripheral
    maximumWriteValueLengthForType:write_type];

  /* limit all writes to the size of one ATT packet (MTU minus the
     ATT header); larger writes would be fragmented by iOS, which
     many BLE UART bridges cannot handle */
  chunk_size = std::clamp(std::min(max_wor, max_wr),
                          std::size_t(20), std::size_t(512));

  const unsigned att_mtu = unsigned(max_wor) + 3;

  [peripheral setNotifyValue:YES forCharacteristic:rx];

  /* the properties reveal e.g. whether the bridge uses (slow,
     acknowledged) indications instead of notifications */
  LogFormat("Bluetooth: %s is ready"
            " (rx=%s props=0x%x tx=%s props=0x%x %s mtu=%u chunk=%u)",
            address.UTF8String,
            rx.UUID.UUIDString.UTF8String, unsigned(rx.properties),
            tx.UUID.UUIDString.UTF8String, unsigned(tx.properties),
            write_type == CBCharacteristicWriteWithoutResponse
            ? "write-without-response" : "write-with-response",
            att_mtu, unsigned(chunk_size));

  if (att_mtu <= 23)
    /* the peer kept the Bluetooth LE default MTU; bulk transfers
       such as IGC flight downloads will be slow */
    LogFormat("Bluetooth: %s negotiated only the minimum ATT MTU;"
              " check the bridge firmware settings for a larger MTU",
              address.UTF8String);

  SetState(PortState::READY);

  /* flush data which was queued while we were connecting */
  SendPendingChunks();
}

void
PortBridge::OnDataReceived(CBCharacteristic *characteristic,
                           NSData *value) noexcept
{
  if (characteristic != rx_characteristic || value.length == 0)
    return;

#ifndef NDEBUG
  /* log the cumulative number of received bytes and notifications,
     at most every five seconds, to help diagnosing stalled or slow
     transfers (the ratio exposes the peer's notification pacing) */
  rx_bytes += value.length;
  ++rx_notifications;
  if (const auto now = std::chrono::steady_clock::now();
      now >= next_rx_log) {
    LogFormat("Bluetooth: %s: %llu bytes in %llu notifications so far",
              address.UTF8String,
              (unsigned long long)rx_bytes,
              (unsigned long long)rx_notifications);
    next_rx_log = now + std::chrono::seconds(5);
  }
#endif

  DataHandler *h;

  {
    const std::lock_guard lock{mutex};
    h = handler;
  }

  if (h != nullptr)
    h->DataReceived({(const std::byte *)value.bytes, value.length});
}

void
PortBridge::OnWriteCompleted(NSError *error) noexcept
{
  write_pending = false;

  if (error != nil) {
    LogFormat("Bluetooth: write to %s failed: %s",
              address.UTF8String, error.localizedDescription.UTF8String);
    Error("Bluetooth write failed");
    return;
  }

  SendPendingChunks();
}

void
PortBridge::OnReadyToSendWriteWithoutResponse() noexcept
{
  SendPendingChunks();
}
