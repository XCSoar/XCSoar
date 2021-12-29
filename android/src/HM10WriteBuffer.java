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

import java.util.Arrays;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.util.Log;

/**
 * This class helps with writing chunked data to a Bluetooth LE HM10
 * device.
 */
final class HM10WriteBuffer {
  private static final String TAG = "XCSoar";

  private static final int MAX_WRITE_CHUNK_SIZE = 20;

  private byte[][] pendingWriteChunks = null;
  private int nextWriteChunkIdx;
  private boolean lastChunkWriteError;

  synchronized boolean beginWriteNextChunk(BluetoothGatt gatt,
                                           BluetoothGattCharacteristic dataCharacteristic) {
    if (pendingWriteChunks == null)
      return false;

    dataCharacteristic.setValue(pendingWriteChunks[nextWriteChunkIdx]);
    if (!gatt.writeCharacteristic(dataCharacteristic)) {
      Log.e(TAG, "GATT characteristic write request failed");
      setError();
      return false;
    }

    ++nextWriteChunkIdx;
    if (nextWriteChunkIdx >= pendingWriteChunks.length) {
      /* writing is done */
      clear();
    }

    return true;
  }

  synchronized void clear() {
    pendingWriteChunks = null;
    notifyAll();
  }

  synchronized void setError() {
    lastChunkWriteError = true;
    clear();
  }

  synchronized boolean drain() {
    final long TIMEOUT = 5000;
    final long waitUntil = System.currentTimeMillis() + TIMEOUT;

    while (pendingWriteChunks != null) {
      final long timeToWait = waitUntil - System.currentTimeMillis();
      if (timeToWait <= 0)
        return false;

      try {
        wait(timeToWait);
      } catch (InterruptedException e) {
        return false;
      }
    }

    return true;
  }

  synchronized int write(BluetoothGatt gatt,
                         BluetoothGattCharacteristic dataCharacteristic,
                         BluetoothGattCharacteristic deviceNameCharacteristic,
                         byte[] data, int length) {
    final long TIMEOUT = 5000;

    if (0 == length)
      return 0;

    if (!drain())
      return 0;

    if ((dataCharacteristic == null) || (deviceNameCharacteristic == null))
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
                                                 i * MAX_WRITE_CHUNK_SIZE,
                                                 Math.min((i + 1) * MAX_WRITE_CHUNK_SIZE,
                                                          length));
    }

    try {
      wait(TIMEOUT);
    } catch (InterruptedException e) {
      /* cancel the write on interruption */
      pendingWriteChunks = null;
      return 0;
    }

    if (pendingWriteChunks != null && nextWriteChunkIdx == 0) {
      /* timeout */
      pendingWriteChunks = null;
      return 0;
    }

    return lastChunkWriteError ? 0 : length;
  }
}
