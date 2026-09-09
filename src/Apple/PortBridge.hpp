// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Device/Port/State.hpp"
#include "thread/Cond.hxx"
#include "thread/Mutex.hxx"
#include "util/StaticFifoBuffer.hxx"

#import <CoreBluetooth/CoreBluetooth.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <span>

@class IOSBluetoothManager;

class PortListener;
class DataHandler;

/**
 * The iOS counterpart of Android's #PortBridge: glue between the C++
 * #Port world and a CoreBluetooth GATT connection.  One instance
 * exists per opened BLE serial port; it is owned by an #ApplePort.
 *
 * The #IOSBluetoothManager keeps a registry of all bridges and
 * forwards CoreBluetooth delegate callbacks to the matching instance;
 * all On*() methods and SendPendingChunks() are invoked on the
 * manager's serial dispatch queue.  The remaining methods may be
 * called from any thread.
 *
 * Writes are buffered and sent in chunks of at most one MTU, pacing
 * the transmission with CoreBluetooth's flow control (see
 * SendPendingChunks()), like Android's HM10WriteBuffer.
 */
class PortBridge final {
  IOSBluetoothManager *const manager;

  NSString *const address;

  /* the following fields are only accessed on the manager's dispatch
     queue */

  CBPeripheral *peripheral = nil;

  /** the characteristic which notifies us about received data */
  CBCharacteristic *rx_characteristic = nil;

  /** the characteristic we write outgoing data to */
  CBCharacteristic *tx_characteristic = nil;

  CBCharacteristicWriteType write_type = CBCharacteristicWriteWithResponse;

  /** maximum number of payload bytes per GATT write */
  std::size_t chunk_size = 20;

  /** number of services with unfinished characteristic discovery */
  NSUInteger pending_characteristic_discoveries = 0;

  /**
   * A write-with-response is in flight; wait for OnWriteCompleted()
   * before sending the next chunk.
   */
  bool write_pending = false;

  /** total number of bytes received, for the throughput log */
  uint64_t rx_bytes = 0;

  /** total number of received notifications, for the throughput log */
  uint64_t rx_notifications = 0;

  /** earliest time OnDataReceived() logs the next throughput line */
  std::chrono::steady_clock::time_point next_rx_log{};

  /* the following fields are protected by the mutex */

  mutable Mutex mutex;
  Cond cond;

  StaticFifoBuffer<std::byte, 4096> write_buffer;

  PortState state = PortState::LIMBO;

  bool closed = false;

  PortListener *listener = nullptr;
  DataHandler *handler = nullptr;

public:
  PortBridge(IOSBluetoothManager *manager, NSString *address) noexcept;

  /**
   * Unregisters this bridge from the manager and disconnects from the
   * peripheral.
   */
  ~PortBridge() noexcept;

  PortBridge(const PortBridge &) = delete;
  PortBridge &operator=(const PortBridge &) = delete;

  NSString *GetAddress() const noexcept {
    return address;
  }

  /**
   * May only be called on the manager's dispatch queue.
   */
  CBPeripheral *GetPeripheral() const noexcept {
    return peripheral;
  }

  void SetListener(PortListener *listener) noexcept;
  void SetInputListener(DataHandler *handler) noexcept;

  [[gnu::pure]]
  PortState GetState() const noexcept;

  /**
   * Wait (with timeout) until all buffered data has been sent.
   *
   * @return false on error or timeout
   */
  bool Drain() noexcept;

  /**
   * Copy data to the write buffer.  Blocks (with timeout) while the
   * buffer is full.
   *
   * @return the number of bytes accepted
   */
  std::size_t Write(std::span<const std::byte> src) noexcept;

  /* callbacks invoked by #IOSBluetoothManager on its dispatch
     queue */

  void OnPeripheralAttached(CBPeripheral *peripheral) noexcept;

  /**
   * The attached peripheral object has become invalid because
   * Bluetooth was switched off; forget it, so that a fresh instance
   * gets attached (by retrieval or by scan) once Bluetooth is back.
   */
  void OnPeripheralLost() noexcept;
  void OnConnected() noexcept;
  void OnDisconnected() noexcept;
  void OnServicesDiscovered(NSError *error) noexcept;
  void OnCharacteristicsDiscovered(NSError *error) noexcept;
  void OnDataReceived(CBCharacteristic *characteristic,
                      NSData *value) noexcept;
  void OnWriteCompleted(NSError *error) noexcept;
  void OnReadyToSendWriteWithoutResponse() noexcept;

  /**
   * Send as many chunks from the write buffer as flow control
   * currently permits.
   */
  void SendPendingChunks() noexcept;

private:
  void SetState(PortState new_state) noexcept;

  /**
   * Report an error to the #PortListener and enter the FAILED state.
   */
  void Error(const char *msg) noexcept;

  /**
   * Pick the RX/TX characteristics after service/characteristic
   * discovery has finished.
   */
  void SelectCharacteristics() noexcept;

  [[gnu::pure]]
  bool CanSendChunk() const noexcept;
};
