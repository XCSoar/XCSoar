// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Arrays;

import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothStatusCodes;
import android.os.Build;
import android.util.Log;

/**
 * This class helps with writing chunked data to a Bluetooth LE HM10
 * device.
 */
final class HM10WriteBuffer {
  private static final String TAG = "XCSoar";

  private static final int BUFFER_SIZE = 256;

  /**
   * The current mtu.  May be updated by
   * BluetoothGattCallback.onMtuChanged() (via class HM10Port).
   */
  private int mtu = 20;

  private final byte[] buffer = new byte[BUFFER_SIZE];
  private int head, tail;

  private boolean lastChunkWriteError;

  /**
   * Is the BluetoothGatt object currently busy, i.e. did we call
   * readCharacteristic() or writeCharacteristic() and are we waiting
   * for a beginWriteNextChunk() call from HM10Port?
   *
   * We need to track this because only one pending
   * readCharacteristic()/writeCharacteristic() operation is allowed,
   * and the second one will fail with
   * BluetoothStatusCodes.ERROR_GATT_WRITE_REQUEST_BUSY.
   */
  private boolean gattBusy = false;

  void setMtu(int _mtu) {
    mtu = _mtu;
  }

  synchronized void reset() {
    lastChunkWriteError = false;
    gattBusy = false;
    clear();
  }

  synchronized boolean beginWriteNextChunk(BluetoothGatt gatt,
                                           BluetoothGattCharacteristic dataCharacteristic) {
    gattBusy = false;

    if (head == tail)
      return false;

    final int nbytes = Math.min(tail - head, mtu);
    final byte[] chunk = Arrays.copyOfRange(buffer, head, head + nbytes);
    head += nbytes;

    boolean success;

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
      int result = gatt.writeCharacteristic(dataCharacteristic, chunk,
                                            dataCharacteristic.WRITE_TYPE_NO_RESPONSE);
      success = result == BluetoothStatusCodes.SUCCESS;
    } else {
      dataCharacteristic.setValue(chunk);
      success = gatt.writeCharacteristic(dataCharacteristic);
    }

    if (!success) {
      Log.e(TAG, "GATT characteristic write request failed");
      setError();
      return false;
    }

    gattBusy = true;

    /* wake up write() or drain() telling them some space in the
       buffer has been freed */
    notifyAll();

    return true;
  }

  private synchronized void clear() {
    head = tail = 0;
    notifyAll();
  }

  synchronized void setError() {
    lastChunkWriteError = true;
    gattBusy = false;
    clear();
  }

  synchronized boolean drain() {
    final long TIMEOUT = 5000;
    final long waitUntil = System.currentTimeMillis() + TIMEOUT;

    if (lastChunkWriteError) {
      /* the last write() failed asynchronously; throw this error now
         so the caller knows something went wrong */
      lastChunkWriteError = false;
      return false;
    }

    while (head != tail) {
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

  /**
   * Wait until there is some free space in the buffer.
   *
   * @return false on error
   */
  synchronized boolean drainSome() {
    final long TIMEOUT = 5000;
    final long waitUntil = System.currentTimeMillis() + TIMEOUT;

    if (lastChunkWriteError) {
      /* the last write() failed asynchronously; throw this error now
         so the caller knows something went wrong */
      lastChunkWriteError = false;
      return false;
    }

    while (head == 0 && tail == BUFFER_SIZE) {
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
    assert(dataCharacteristic != null);
    assert(deviceNameCharacteristic != null);
    assert(length > 0);

    if (!drainSome())
      return 0;

    if (head > 0) {
      /* shift the buffer contents to the beginning to make room at
         the end */
      System.arraycopy(buffer, head, buffer, 0, tail - head);
      tail -= head;
      head = 0;
    }

    final int nbytes = Math.min(length, BUFFER_SIZE - tail);
    System.arraycopy(data, 0, buffer, tail, nbytes);
    tail += nbytes;

    /* Workaround: To avoid a race condition when data is sent and received
       at the same time, we place a read request for the device name
       characteristic here. This way, we can place the actual write
       operation in the read callback so that the write operation is performed
       int the GATT event handling thread. */
    if (!gattBusy) {
      if (!gatt.readCharacteristic(deviceNameCharacteristic)) {
        Log.e(TAG, "GATT characteristic read request failed");
        clear();
        return 0;
      }

      gattBusy = true;
    }

    return nbytes;
  }
}
