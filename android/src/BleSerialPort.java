// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.io.IOException;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.os.Build;
import android.util.Log;

/**
 * An #AndroidPort implementation for BLE serial bridge devices.
 * Supports HM-10, Nordic UART Service, and Microchip/ISSC transparent
 * UART, with auto-detection based on the service UUID.
 */
public class BleSerialPort
    extends BluetoothGattCallback
    implements AndroidPort  {
  private static final String TAG = "XCSoar";

  /* ATT MTU asked at connect and available to a flight download.
     Android 14 and later ignores this and negotiates 517. */
  static final int REQUESTED_MTU = 512;

  /* Maximum number of milliseconds to wait for disconnected state after
     calling BluetoothGatt.disconnect() in close() */
  private static final int DISCONNECT_TIMEOUT = 500;

  private PortListener portListener;
  private volatile InputListener listener;
  private final SafeDestruct safeDestruct = new SafeDestruct();

  private BluetoothGatt gatt;

  /* For NUS/ISSC, RX characteristic. For HM-10, RX and TX use the same
     one. */
  private BluetoothGattCharacteristic writeCharacteristic;

  private BluetoothGattCharacteristic notifyCharacteristic;

  /* For NUS/ISSC, stays null. For HM-10, used as a workaround to avoid
     a race condition. */
  private BluetoothGattCharacteristic deviceNameCharacteristic;

  private volatile boolean shutdown = false;

  private final BleSerialWriteBuffer writeBuffer = new BleSerialWriteBuffer();

  private volatile int portState = STATE_LIMBO;

  private final Object gattStateSync = new Object();
  private int gattState = BluetoothGatt.STATE_DISCONNECTED;

  private boolean setupCharacteristicsPending = false;

  /**
   * Private constructor. All fields are initialized to their default values.
   * Use create() factory method to instantiate.
   */
  private BleSerialPort() {
    // All fields are initialized to their default values
  }

  /**
   * Factory method to create and initialize BleSerialPort.
   * This pattern avoids the this-escape warning by ensuring
   * the object is fully constructed before passing it to connectGatt().
   */
  public static BleSerialPort create(Context context, BluetoothDevice device)
    throws IOException
  {
    BleSerialPort port = new BleSerialPort();

    // Now that the object is fully constructed, we can safely pass it
    BluetoothGatt connectedGatt;
    if (Build.VERSION.SDK_INT >= 23)
      connectedGatt = device.connectGatt(context, true, port, BluetoothDevice.TRANSPORT_LE);
    else
      connectedGatt = device.connectGatt(context, true, port);

    if (connectedGatt == null)
      throw new IOException("Bluetooth GATT connect failed");

    // Assign to field after successful connection
    port.gatt = connectedGatt;

    return port;
  }

  private void findCharacteristics() throws Error {
    writeCharacteristic = null;
    notifyCharacteristic = null;
    deviceNameCharacteristic = null;

    /* Prefer Nordic UART Service if available */
    BluetoothGattService service = gatt.getService(BluetoothUuids.NORDIC_UART_SERVICE);
    if (service != null) {
      writeCharacteristic =
        service.getCharacteristic(BluetoothUuids.NORDIC_UART_RX_CHARACTERISTIC);
      notifyCharacteristic =
        service.getCharacteristic(BluetoothUuids.NORDIC_UART_TX_CHARACTERISTIC);
      /* deviceNameCharacteristic stays null */
    } else if ((service = gatt.getService(BluetoothUuids.ISSC_UART_SERVICE)) != null) {
      writeCharacteristic =
        service.getCharacteristic(BluetoothUuids.ISSC_UART_RX_CHARACTERISTIC);
      notifyCharacteristic =
        service.getCharacteristic(BluetoothUuids.ISSC_UART_TX_CHARACTERISTIC);
      /* deviceNameCharacteristic stays null */
    } else {
      service = gatt.getService(BluetoothUuids.HM10_SERVICE);
      if (service != null) {
        writeCharacteristic =
          service.getCharacteristic(BluetoothUuids.HM10_RX_TX_CHARACTERISTIC);
        notifyCharacteristic = writeCharacteristic;

        BluetoothGattService genericAccess =
          gatt.getService(BluetoothUuids.GENERIC_ACCESS_SERVICE);
        if (genericAccess != null) {
          deviceNameCharacteristic =
            genericAccess.getCharacteristic(BluetoothUuids.DEVICE_NAME_CHARACTERISTIC);
        }

        if (deviceNameCharacteristic == null)
          throw new Error("GATT device name characteristic not found");
      }
    }

    if (writeCharacteristic == null || notifyCharacteristic == null)
      throw new Error("BLE serial service not found");
  }

  private void setupCharacteristics() throws Error {
    findCharacteristics();

    if (!gatt.setCharacteristicNotification(notifyCharacteristic, true))
      throw new Error("Could not enable GATT characteristic notification");

    BluetoothGattDescriptor descriptor =
      notifyCharacteristic.getDescriptor(BluetoothUuids.CLIENT_CHARACTERISTIC_CONFIGURATION);
    if (descriptor == null) {
      Log.e(TAG, "BLE notify characteristic missing CCCD descriptor");
      throw new Error("GATT client characteristic configuration descriptor not found");
    }
    descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
    gatt.writeDescriptor(descriptor);
    portState = STATE_READY;
    stateChanged();
  }

  @Override
  public void onConnectionStateChange(BluetoothGatt gatt,
      int status,
      int newState) {
    try {
      if (BluetoothProfile.STATE_CONNECTED == newState) {
        if (!gatt.discoverServices())
          throw new Error("Discovering GATT services request failed");
      } else {
        writeCharacteristic = null;
        notifyCharacteristic = null;
        deviceNameCharacteristic = null;

        if ((BluetoothProfile.STATE_DISCONNECTED == newState) && !shutdown)
          noteDisconnect(gatt, status);
      }

      portState = STATE_LIMBO;
    } catch (Error e) {
      error(e.getMessage());
    }

    writeBuffer.reset();
    stateChanged();
    synchronized (gattStateSync) {
      gattState = newState;
      gattStateSync.notifyAll();
    }
  }

  @Override
  public void onServicesDiscovered(BluetoothGatt gatt,
                                   int status) {
    try {
      if (BluetoothGatt.GATT_SUCCESS != status)
        throw new Error("Discovering GATT services failed");

      /* Postpone setupCharacteristics() until onMtuChanged().
         A peripheral that rejects the request (HM-10 is often 23
         bytes) still calls onMtuChanged(); if the stack will not
         start the request, continue at the default MTU. */
      setupCharacteristicsPending = true;
      if (!requestMtu(REQUESTED_MTU)) {
        setupCharacteristicsPending = false;
        setupCharacteristics();
      }
    } catch (Error e) {
      error(e.getMessage());
      stateChanged();
    }
  }

  @Override
  public void onCharacteristicRead(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic, int status) {
    writeBuffer.beginWriteNextChunk(gatt, writeCharacteristic);
  }

  @Override
  public void onCharacteristicWrite(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic, int status) {
    if (BluetoothGatt.GATT_SUCCESS == status) {
      writeBuffer.beginWriteNextChunk(gatt, writeCharacteristic);
    } else {
      Log.e(TAG, "GATT characteristic write failed");
      writeBuffer.setError();
    }
  }

  @Override
  public void onCharacteristicChanged(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic) {
    if ((notifyCharacteristic != null) &&
        (notifyCharacteristic.getUuid().equals(characteristic.getUuid()))) {
      if (listener != null && safeDestruct.increment()) {
        try {
          byte[] data = characteristic.getValue();
          listener.dataReceived(data, data.length);;
        } finally {
          safeDestruct.decrement();
        }
      }
    }
  }

  @Override
  public void onMtuChanged(BluetoothGatt gatt, int mtu, int status) {
    super.onMtuChanged(gatt, mtu, status);

    /* the MTU reported by the Android APIs is the ATT MTU, not the
       data MTU; from this reported MTU value, we have to subtract the
       ATT header size (1 byte opcode, 2 bytes handle) to get the data
       MTU */
    final int ATT_HEADER_SIZE = 3;

    if (status == BluetoothGatt.GATT_SUCCESS && mtu > ATT_HEADER_SIZE)
      writeBuffer.setMtu(mtu - ATT_HEADER_SIZE);

    if (setupCharacteristicsPending) {
      setupCharacteristicsPending = false;

      try {
        setupCharacteristics();
      } catch (Error e) {
        error(e.getMessage());
        stateChanged();
      }
    }
  }

  @Override public void setListener(PortListener _listener) {
    portListener = _listener;
  }

  @Override
  public void setInputListener(InputListener _listener) {
    listener = _listener;
  }

  @Override
  public void close() {
    safeDestruct.beginShutdown();

    shutdown = true;
    writeBuffer.reset();
    gatt.disconnect();
    synchronized (gattStateSync) {
      long waitUntil = System.currentTimeMillis() + DISCONNECT_TIMEOUT;
      while (gattState != BluetoothGatt.STATE_DISCONNECTED) {
        long timeToWait = waitUntil - System.currentTimeMillis();
        if (timeToWait <= 0) {
          break;
        }
        try {
          gattStateSync.wait(timeToWait);
        } catch (InterruptedException e) {
          break;
        }
      }
    }
    gatt.close();

    safeDestruct.finishShutdown();
  }

  @Override
  public int getState() {
    return portState;
  }

  @Override
  public boolean drain() {
    return writeBuffer.drain();
  }

  @Override
  public int getBaudRate() {
    return 0;
  }

  @Override
  public boolean setBaudRate(int baud) {
    return true;
  }

  @Override
  public boolean setConnectionPriority(int priority) {
    final BluetoothGatt gatt = this.gatt;
    if (gatt == null || shutdown)
      return false;

    /* Reject anything outside the three public constants.
       requestConnectionPriority() throws on an unknown value. */
    if (priority < BluetoothGatt.CONNECTION_PRIORITY_BALANCED ||
        priority > BluetoothGatt.CONNECTION_PRIORITY_LOW_POWER)
      return false;

    return gatt.requestConnectionPriority(priority);
  }

  @Override
  public boolean requestMtu(int mtu) {
    final BluetoothGatt gatt = this.gatt;
    /* 23 is the default ATT MTU.  517 is Android's maximum. */
    if (gatt == null || shutdown || mtu < 23 || mtu > 517)
      return false;

    return gatt.requestMtu(mtu);
  }

  @Override
  public int write(byte[] data, int length) {
    if (0 == length)
      return 0;

    if (portState != STATE_READY)
      return 0;

    assert(writeCharacteristic != null);

    return writeBuffer.write(gatt, writeCharacteristic,
                             deviceNameCharacteristic,
                             data, length);
  }

  /**
   * Log a GATT drop without failing the port.  portError() marks the
   * port FAILED, and the ticker then closes it, which drops the
   * gatt.connect() this method just queued.
   *
   * status 0 is a clean drop, 8 a supervision timeout, 19 the peer,
   * 133 the Android stack.
   */
  private void noteDisconnect(BluetoothGatt gatt, int status) {
    final String msg = "GATT disconnected status=" + status;
    if (gatt.connect()) {
      log(msg + ", reconnecting");
      return;
    }

    throw new Error(msg + ", reconnect failed");
  }

  private void log(String msg) {
    PortListener portListener = this.portListener;
    if (portListener != null && safeDestruct.increment()) {
      try {
        portListener.portLog(msg);
      } finally {
        safeDestruct.decrement();
      }
    } else
      Log.w(TAG, msg);
  }

  protected final void stateChanged() {
    PortListener portListener = this.portListener;
    if (portListener != null && safeDestruct.increment()) {
      try {
        portListener.portStateChanged();
      } finally {
        safeDestruct.decrement();
      }
    }
  }

  protected void error(String msg) {
    portState = STATE_FAILED;

    PortListener portListener = this.portListener;
    if (portListener != null && safeDestruct.increment()) {
      try {
        portListener.portError(msg);
      } finally {
        safeDestruct.decrement();
      }
    }
  }

  static class Error extends Exception {
    private static final long serialVersionUID = -2699261433374751557L;

    public Error(String message) {
      super(message);
    }

    public Error(String message, Throwable cause) {
      super(message, cause);
    }
  }
}
