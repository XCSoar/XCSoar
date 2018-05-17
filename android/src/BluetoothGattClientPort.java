/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

import java.util.Arrays;
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

  private BluetoothDevice device;
  private BluetoothGatt gatt;
  private BluetoothGattCharacteristic dataCharacteristic;
  private BluetoothGattCharacteristic deviceNameCharacteristic;
  private volatile boolean shutdown = false;

  private final Object writeChunksSync = new Object();
  private byte[][] pendingWriteChunks = null;
  private int nextWriteChunkIdx;
  private boolean lastChunkWriteError;

  private volatile int portState = STATE_LIMBO;

  private final Object gattStateSync = new Object();
  private int gattState = BluetoothGatt.STATE_DISCONNECTED;

  public BluetoothGattClientPort(BluetoothDevice _device) {
    device = _device;
  }

  public void startConnect(Context context) throws IOException {
    shutdown = false;
    gatt = device.connectGatt(context, false, this);
    if (gatt == null)
      throw new IOException("Bluetooth GATT connect failed");
  }

  private boolean findCharacteristics() {
    try {
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

      if (dataCharacteristic == null) {
        Log.e(TAG, "GATT data characteristic not found");
        return false;
      }

      if (deviceNameCharacteristic == null) {
        Log.e(TAG, "GATT device name characteristic not found");
        return false;
      }

      return true;
    } catch (Exception e) {
      Log.e(TAG, "GATT characteristics lookup failed", e);
      return false;
    }
  }

  private boolean beginWriteNextChunk() {
    synchronized (writeChunksSync) {
      if ((pendingWriteChunks == null)
          || (nextWriteChunkIdx < 0)
          || (pendingWriteChunks.length <= nextWriteChunkIdx)) return false;
      dataCharacteristic.setValue(pendingWriteChunks[nextWriteChunkIdx]);
      ++nextWriteChunkIdx;
      if (gatt.writeCharacteristic(dataCharacteristic)) {
        return true;
      } else {
        Log.e(TAG, "GATT characteristic write request failed");
        lastChunkWriteError = true;
        writeChunksSync.notifyAll();
        return false;
      }
    }
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
    synchronized (writeChunksSync) {
      pendingWriteChunks = null;
      writeChunksSync.notifyAll();
    }
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
    if (BluetoothGatt.GATT_SUCCESS == status) {
      if (findCharacteristics()) {
        if (gatt.setCharacteristicNotification(dataCharacteristic, true)) {
          BluetoothGattDescriptor descriptor =
            dataCharacteristic.getDescriptor(RX_TX_DESCRIPTOR_UUID);
          descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
          gatt.writeDescriptor(descriptor);
          portState = STATE_READY;
        } else {
          Log.e(TAG, "Could not enable GATT characteristic notification");
          portState = STATE_FAILED;
        }
      } else {
        portState = STATE_FAILED;
      }
    } else {
      Log.e(TAG, "Discovering GATT services failed");
      portState = STATE_FAILED;
    }
    stateChanged();
  }

  @Override
  public void onCharacteristicRead(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic, int status) {
    Log.e(TAG, "GATT characteristic read");
    synchronized (writeChunksSync) {
      if ((pendingWriteChunks != null) && !beginWriteNextChunk()) {
        pendingWriteChunks = null;
      }
    }
  }

  @Override
  public void onCharacteristicWrite(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic, int status) {
    synchronized (writeChunksSync) {
      if (BluetoothGatt.GATT_SUCCESS == status) {
        if (!beginWriteNextChunk()) {
          pendingWriteChunks = null;
        }
      } else {
        Log.e(TAG, "GATT characteristic write failed");
        lastChunkWriteError = true;
        pendingWriteChunks = null;
      }

      writeChunksSync.notifyAll();
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
    synchronized (writeChunksSync) {
      pendingWriteChunks = null;
      writeChunksSync.notifyAll();
    }
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
    synchronized (writeChunksSync) {
      while (pendingWriteChunks != null) {
        try {
          writeChunksSync.wait();
        } catch (InterruptedException e) {
          return false;
        }
      }
      return true;
    }
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
    synchronized (writeChunksSync) {
      if (0 == length)
        return 0;
      if ((dataCharacteristic == null) || (deviceNameCharacteristic == null))
        return 0;
      if ((pendingWriteChunks != null) && !drain())
        return 0;

      /* Workaround: To avoid a race condition when data is sent and received
         at the same time, we place a read request for the device name
         characteristic here. This way, we can place the actual write
         operation in the read callback so that the write operation is performed
         int the GATT event handling thread. */
      if (!gatt.readCharacteristic(deviceNameCharacteristic)) {
        Log.e(TAG, "GATT characteristic read request failed");
        return 0;
      }

      /* Write data in 20 byte large chunks at maximun. Most GATT devices do
         not support characteristic values which are larger than 20 bytes. */
      int writeChunksCount = (length + MAX_WRITE_CHUNK_SIZE - 1)
          / MAX_WRITE_CHUNK_SIZE;
      pendingWriteChunks = new byte[writeChunksCount][];
      nextWriteChunkIdx = 0;
      lastChunkWriteError = false;
      for (int i = 0; i < writeChunksCount; ++i) {
        pendingWriteChunks[i] = Arrays.copyOfRange(data,
            i * MAX_WRITE_CHUNK_SIZE, Math.min((i + 1) * MAX_WRITE_CHUNK_SIZE,
            length));
      }

      try {
        writeChunksSync.wait();
      } catch (InterruptedException e) {
        return 0;
      }

      return lastChunkWriteError ? 0 : length;
    }
  }

  protected final void stateChanged() {
    PortListener portListener = this.portListener;
    if (portListener != null)
      portListener.portStateChanged();
  }
}
