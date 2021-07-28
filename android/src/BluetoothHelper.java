/*
Copyright_License {

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

import java.util.UUID;
import java.util.Set;
import java.util.Collection;
import java.util.List;
import java.util.LinkedList;
import java.util.Map;
import java.util.TreeMap;
import java.io.IOException;

import android.os.ParcelUuid;
import android.util.Log;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanRecord;
import android.content.Context;
import android.content.pm.PackageManager;

/**
 * A library that constructs Bluetooth ports.  It is called by C++
 * code.
 */
final class BluetoothHelper
  extends ScanCallback
{
  private static final String TAG = "XCSoar";
  private static final UUID THE_UUID =
        UUID.fromString("00001101-0000-1000-8000-00805F9B34FB");

  private final Context context;

  private final BluetoothAdapter adapter;

  private BluetoothLeScanner scanner;

  /**
   * Does this device support Bluetooth Low Energy?
   */
  private final boolean hasLe;

  private final Collection<DetectDeviceListener> detectListeners =
    new LinkedList<DetectDeviceListener>();

  BluetoothHelper(Context context) throws Exception {
    this.context = context;

    BluetoothManager manager = (BluetoothManager)
      context.getSystemService(Context.BLUETOOTH_SERVICE);
    if (manager == null)
      throw new Exception("No Bluetooth manager");

    adapter = manager.getAdapter();
    if (adapter == null)
      throw new Exception("No Bluetooth adapter found");

    hasLe = context.getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE);
  }

  public boolean isEnabled() {
    return adapter.isEnabled();
  }

  /**
   * Turns the #BluetoothDevice into a human-readable string.
   */
  public static String getDisplayString(BluetoothDevice device) {
    String name = device.getName();
    String address = device.getAddress();

    if (name == null)
      return address;

    return name + " [" + address + "]";
  }

  public static String getDisplayString(BluetoothSocket socket) {
    return getDisplayString(socket.getRemoteDevice());
  }

  public String getNameFromAddress(String address) {
    try {
      return adapter.getRemoteDevice(address).getName();
    } catch (Exception e) {
      Log.e(TAG, "Failed to look up name of " + address, e);
      return null;
    }
  }

  public synchronized void addDetectDeviceListener(DetectDeviceListener l) {
    detectListeners.add(l);

    Set<BluetoothDevice> devices = adapter.getBondedDevices();
    if (devices != null)
      for (BluetoothDevice device : devices)
        l.onDeviceDetected(device.getType() == BluetoothDevice.DEVICE_TYPE_LE
                           ? DetectDeviceListener.TYPE_BLUETOOTH_LE
                           : DetectDeviceListener.TYPE_BLUETOOTH_CLASSIC,
                           device.getAddress(), device.getName(),
                           0);

    if (hasLe && scanner == null) {
      try {
        scanner = adapter.getBluetoothLeScanner();
        if (scanner != null)
          scanner.startScan(this);
      } catch (Exception e) {
        Log.e(TAG, "Bluetooth LE scan failed", e);
        scanner = null;
      }
    }
  }

  public synchronized void removeDetectDeviceListener(DetectDeviceListener l) {
    detectListeners.remove(l);

    if (!detectListeners.isEmpty())
      return;

    if (scanner != null) {
      scanner.stopScan(this);
      scanner = null;
    }
  }

  public BluetoothSensor connectSensor(String address, SensorListener listener)
    throws IOException
  {
    if (!hasLe)
      throw new IOException("No Bluetooth LE support");

    BluetoothDevice device = adapter.getRemoteDevice(address);
    if (device == null)
      throw new IOException("Bluetooth device not found");

    return new BluetoothSensor(context, device, listener);
  }

  public AndroidPort connectHM10(String address)
    throws IOException {
    if (!hasLe)
      throw new IOException("No Bluetooth adapter found");

    BluetoothDevice device = adapter.getRemoteDevice(address);
    if (device == null)
      throw new IOException("Bluetooth device not found");

    Log.d(TAG, String.format("Bluetooth device \"%s\" (%s) is a LE device, trying to connect using GATT...",
                             device.getName(), device.getAddress()));
    return new HM10Port(context, device);
  }

  public AndroidPort connect(String address)
    throws IOException {
    BluetoothDevice device = adapter.getRemoteDevice(address);
    if (device == null)
      throw new IOException("Bluetooth device not found");

    BluetoothSocket socket =
      device.createRfcommSocketToServiceRecord(THE_UUID);
    return new BluetoothClientPort(socket);
  }

  public AndroidPort createServer() throws IOException {
    return new BluetoothServerPort(adapter, THE_UUID);
  }

  /**
   * Identify the detected service UUIDs and convert it to a feature
   * flag bit set.
   */
  private static long getFeatures(Collection<ParcelUuid> serviceUuids) {
    long features = 0;

    for (ParcelUuid puuid : serviceUuids) {
      UUID uuid = puuid.getUuid();
      if (BluetoothUuids.HM10_SERVICE.equals(uuid))
        features |= DetectDeviceListener.FEATURE_HM10;
      else if (BluetoothUuids.HEART_RATE_SERVICE.equals(uuid))
        features |= DetectDeviceListener.FEATURE_HEART_RATE;
      else if (BluetoothUuids.FLYTEC_SENSBOX_SERVICE.equals(uuid))
        features |= DetectDeviceListener.FEATURE_FLYTEC_SENSBOX;
    }

    return features;
  }

  private static long getFeatures(ScanRecord record) {
    Collection<ParcelUuid> serviceUuids = record.getServiceUuids();
    return serviceUuids != null
      ? getFeatures(serviceUuids)
      : 0;
  }

  private static long getFeatures(ScanResult result) {
    ScanRecord record = result.getScanRecord();
    return record != null
      ? getFeatures(record)
      : 0;
  }

  private synchronized void broadcastScanResult(ScanResult result) {
    BluetoothDevice device = result.getDevice();
    long features = getFeatures(result);

    for (DetectDeviceListener l : detectListeners)
      l.onDeviceDetected(DetectDeviceListener.TYPE_BLUETOOTH_LE,
                         device.getAddress(), device.getName(),
                         features);
  }

  @Override
  public void onScanResult(int callbackType, ScanResult result) {
    broadcastScanResult(result);
  }

  @Override
  public void onBatchScanResults(List<ScanResult> results) {
    for (ScanResult r : results)
      broadcastScanResult(r);
  }

  @Override
  public void onScanFailed(int errorCode) {
    Log.e(TAG, "Bluetooth LE scan failed with error code " + errorCode);
  }
}
