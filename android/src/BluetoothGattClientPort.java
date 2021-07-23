/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

package org.xcsoar;

import java.util.List;
import java.util.UUID;
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
 * AndroidPort implementation for Bluetooth Low Energy devices using the
 * GATT protocol.
 */
public class BluetoothGattClientPort
    extends BluetoothGattCallback
    implements AndroidPort  {
  private static final String TAG = "XCSoar";

  /**
   * The HM-10 and compatible bluetooth modules use a GATT characteristic
   * with this UUID for sending and receiving data.
   */
  private static final UUID RX_TX_CHARACTERISTIC_UUID =
      UUID.fromString("0000FFE1-0000-1000-8000-00805F9B34FB");
  private static final UUID DEVICE_NAME_CHARACTERISTIC_UUID =
      UUID.fromString("00002A00-0000-1000-8000-00805F9B34FB");
  private static final UUID RX_TX_DESCRIPTOR_UUID =
      UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");

  private static final int MAX_WRITE_CHUNK_SIZE = 20;

  /* Maximum number of milliseconds to wait for disconnected state after
     calling BluetoothGatt.disconnect() in close() */
  private static final int DISCONNECT_TIMEOUT = 500;

  private PortListener portListener;
  private volatile InputListener listener;

  private final BluetoothGatt gatt;
  private BluetoothGattCharacteristic dataCharacteristic;
  private BluetoothGattCharacteristic deviceNameCharacteristic;
  private volatile boolean shutdown = false;

  private final HM10WriteBuffer writeBuffer = new HM10WriteBuffer();

  private volatile int portState = STATE_LIMBO;

  private final Object gattStateSync = new Object();
  private int gattState = BluetoothGatt.STATE_DISCONNECTED;

  public BluetoothGattClientPort(Context context, BluetoothDevice device)
    throws IOException
  {
    if (Build.VERSION.SDK_INT >= 23)
      gatt = device.connectGatt(context, false, this, BluetoothDevice.TRANSPORT_LE);
    else
      gatt = device.connectGatt(context, false, this);

    if (gatt == null)
      throw new IOException("Bluetooth GATT connect failed");
  }

  private void findCharacteristics() throws Error {
    dataCharacteristic = null;
    deviceNameCharacteristic = null;

    List<BluetoothGattService> services = gatt.getServices();
    if (services != null) {
      for (BluetoothGattService gattService : services) {
        for (BluetoothGattCharacteristic characteristic :
               gattService.getCharacteristics()) {
          if (RX_TX_CHARACTERISTIC_UUID.equals(
                                               characteristic.getUuid())) {
            dataCharacteristic = characteristic;
          } else if (DEVICE_NAME_CHARACTERISTIC_UUID.equals(
                                                            characteristic.getUuid())) {
            deviceNameCharacteristic = characteristic;
          }
        }
      }
    }

    if (dataCharacteristic == null)
      throw new Error("GATT data characteristic not found");

    if (deviceNameCharacteristic == null)
      throw new Error("GATT device name characteristic not found");
  }

  @Override
  public void onConnectionStateChange(BluetoothGatt gatt,
      int status,
      int newState) {
    int newPortState = STATE_LIMBO;
    if (BluetoothProfile.STATE_CONNECTED == newState) {
      if (!gatt.discoverServices()) {
        Log.e(TAG, "Discovering GATT services request failed");
        newPortState = STATE_FAILED;
      }
    } else {
      dataCharacteristic = null;
      deviceNameCharacteristic = null;
      if ((BluetoothProfile.STATE_DISCONNECTED == newState) && !shutdown) {
        if (!gatt.connect()) {
          Log.w(TAG,
              "Received GATT disconnected event, and reconnect attempt failed");
          newPortState = STATE_FAILED;
        }
      }
    }
    writeBuffer.clear();
    portState = newPortState;
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

      findCharacteristics();

      if (!gatt.setCharacteristicNotification(dataCharacteristic, true))
        throw new Error("Could not enable GATT characteristic notification");

      BluetoothGattDescriptor descriptor =
        dataCharacteristic.getDescriptor(RX_TX_DESCRIPTOR_UUID);
      descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
      gatt.writeDescriptor(descriptor);
      portState = STATE_READY;
    } catch (Error e) {
      error(e.getMessage());
      portState = STATE_FAILED;
    } finally {
      stateChanged();
    }
  }

  @Override
  public void onCharacteristicRead(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic, int status) {
    Log.e(TAG, "GATT characteristic read");
    writeBuffer.beginWriteNextChunk(gatt, dataCharacteristic);
  }

  @Override
  public void onCharacteristicWrite(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic, int status) {
    synchronized (writeBuffer) {
      if (BluetoothGatt.GATT_SUCCESS == status) {
        writeBuffer.beginWriteNextChunk(gatt, dataCharacteristic);
        writeBuffer.notifyAll();
      } else {
        Log.e(TAG, "GATT characteristic write failed");
        writeBuffer.setError();
      }
    }
  }

  @Override
  public void onCharacteristicChanged(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic) {
    if ((dataCharacteristic != null) &&
        (dataCharacteristic.getUuid().equals(characteristic.getUuid()))) {
      if (listener != null) {
        byte[] data = characteristic.getValue();
        listener.dataReceived(data, data.length);;
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
    shutdown = true;
    writeBuffer.clear();
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
  public int write(byte[] data, int length) {
    if (0 == length)
      return 0;

    return writeBuffer.write(gatt, dataCharacteristic,
                             deviceNameCharacteristic,
                             data, length);
  }

  protected final void stateChanged() {
    PortListener portListener = this.portListener;
    if (portListener != null)
      portListener.portStateChanged();
  }

  protected void error(String msg) {
    PortListener portListener = this.portListener;
    if (portListener != null)
      portListener.portError(msg);
  }

  static class Error extends Exception {
    public Error(String message) {
      super(message);
    }

    public Error(String message, Throwable cause) {
      super(message, cause);
    }
  }
}
