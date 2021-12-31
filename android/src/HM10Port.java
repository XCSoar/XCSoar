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
 * An #AndroidPort implementation for connecting to a HM-10 (Bluetooth
 * LE).
 */
public class HM10Port
    extends BluetoothGattCallback
    implements AndroidPort  {
  private static final String TAG = "XCSoar";

  private static final int MAX_WRITE_CHUNK_SIZE = 20;

  /* Maximum number of milliseconds to wait for disconnected state after
     calling BluetoothGatt.disconnect() in close() */
  private static final int DISCONNECT_TIMEOUT = 500;

  private PortListener portListener;
  private volatile InputListener listener;
  private final SafeDestruct safeDestruct = new SafeDestruct();

  private final BluetoothGatt gatt;
  private BluetoothGattCharacteristic dataCharacteristic;
  private BluetoothGattCharacteristic deviceNameCharacteristic;
  private volatile boolean shutdown = false;

  private final HM10WriteBuffer writeBuffer = new HM10WriteBuffer();

  private volatile int portState = STATE_LIMBO;

  private final Object gattStateSync = new Object();
  private int gattState = BluetoothGatt.STATE_DISCONNECTED;

  public HM10Port(Context context, BluetoothDevice device)
    throws IOException
  {
    if (Build.VERSION.SDK_INT >= 23)
      gatt = device.connectGatt(context, true, this, BluetoothDevice.TRANSPORT_LE);
    else
      gatt = device.connectGatt(context, true, this);

    if (gatt == null)
      throw new IOException("Bluetooth GATT connect failed");
  }

  private void findCharacteristics() throws Error {
    dataCharacteristic = null;
    deviceNameCharacteristic = null;

    BluetoothGattService service = gatt.getService(BluetoothUuids.HM10_SERVICE);
    if (service != null) {
      dataCharacteristic = service.getCharacteristic(BluetoothUuids.HM10_RX_TX_CHARACTERISTIC);
    }

    service = gatt.getService(BluetoothUuids.GENERIC_ACCESS_SERVICE);
    if (service != null) {
      deviceNameCharacteristic = service.getCharacteristic(BluetoothUuids.DEVICE_NAME_CHARACTERISTIC);
    }

    if (dataCharacteristic == null)
      throw new Error("HM10 data characteristic not found");

    if (deviceNameCharacteristic == null)
      throw new Error("GATT device name characteristic not found");
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
        dataCharacteristic = null;
        deviceNameCharacteristic = null;

        if ((BluetoothProfile.STATE_DISCONNECTED == newState) && !shutdown &&
            !gatt.connect())
          throw new Error("Received GATT disconnected event, and reconnect attempt failed");
      }

      portState = STATE_LIMBO;
    } catch (Error e) {
      error(e.getMessage());
    }

    writeBuffer.clear();
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
        dataCharacteristic.getDescriptor(BluetoothUuids.CLIENT_CHARACTERISTIC_CONFIGURATION);
      descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
      gatt.writeDescriptor(descriptor);
      portState = STATE_READY;
    } catch (Error e) {
      error(e.getMessage());
    } finally {
      stateChanged();
    }
  }

  @Override
  public void onCharacteristicRead(BluetoothGatt gatt,
      BluetoothGattCharacteristic characteristic, int status) {
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
  public int write(byte[] data, int length) {
    if (0 == length)
      return 0;

    return writeBuffer.write(gatt, dataCharacteristic,
                             deviceNameCharacteristic,
                             data, length);
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
