// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.UUID;
import java.util.Set;
import java.util.Collection;
import java.util.List;
import java.util.LinkedList;
import java.io.IOException;

import android.os.ParcelUuid;
import android.util.Log;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanRecord;
import android.content.Context;
import android.content.pm.PackageManager;
import android.Manifest;

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
  private final PermissionManager permissionManager;

  private final BluetoothAdapter adapter;

  private BluetoothLeScanner scanner;

  /**
   * Does this device support Bluetooth Low Energy?
   */
  private final boolean hasLe;

  private final Collection<DetectDeviceListener> detectListeners =
    new LinkedList<DetectDeviceListener>();

  BluetoothHelper(Context context, PermissionManager permissionManager) throws Exception {
    this.context = context;
    this.permissionManager = permissionManager;

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
    try {
      return adapter.isEnabled();
    } catch (SecurityException e) {
      // only Android R or older
      return false;
    }
  }

  /**
   * Wrapper for BluetoothDevice.getName() which catches
   * SecurityException and returns null in this case.
   */
  private static String getName(BluetoothDevice device) {
    try {
      return device.getName();
    } catch (SecurityException e) {
      return null;
    }
  }

  /**
   * Turns the #BluetoothDevice into a human-readable string.
   */
  public static String getDisplayString(BluetoothDevice device) {
    String name = getName(device);
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
      return getName(adapter.getRemoteDevice(address));
    } catch (IllegalArgumentException e) {
      // address is malformed
      return null;
    }
  }

  private static void submitBondedDevices(Collection<BluetoothDevice> devices,
                                          DetectDeviceListener l) {
    for (BluetoothDevice device : devices)
      l.onDeviceDetected(device.getType() == BluetoothDevice.DEVICE_TYPE_LE
                         ? DetectDeviceListener.TYPE_BLUETOOTH_LE
                         : DetectDeviceListener.TYPE_BLUETOOTH_CLASSIC,
                         device.getAddress(), getName(device),
                         0);
  }

  private synchronized void broadcastBondedDevices(Collection<BluetoothDevice> devices) {
    for (DetectDeviceListener l : detectListeners)
      submitBondedDevices(devices, l);
  }

  private final boolean requestScanPermission(PermissionManager.PermissionHandler handler) {
    /* this permission was introduced in Android 12 and is granted
       implicitly in older versions */
    return android.os.Build.VERSION.SDK_INT < 31 ||
      permissionManager.requestPermission(Manifest.permission.BLUETOOTH_SCAN,
                                          handler);
  }

  private final boolean requestConnectPermission(PermissionManager.PermissionHandler handler) {
    /* this permission was introduced in Android 12 and is granted
       implicitly in older versions */
    return android.os.Build.VERSION.SDK_INT < 31 ||
      permissionManager.requestPermission(Manifest.permission.BLUETOOTH_CONNECT,
                                          handler);
  }

  private final PermissionManager.PermissionHandler bondedPermissionHandler =
    new PermissionManager.PermissionHandler() {
      @Override
      public void onRequestPermissionsResult(boolean granted) {
        if (!granted)
          return;

        /* try again */
        try {
          Set<BluetoothDevice> devices = adapter.getBondedDevices();
          if (devices != null)
            broadcastBondedDevices(devices);
        } catch (SecurityException e) {
          // we still don't have permission.BLUETOOTH_CONNECT??
          Log.e(TAG, "Cannot list bonded Bluetooth devices", e);
        }
      }
    };

  private synchronized void startLeScan() {
    if (scanner != null || detectListeners.isEmpty())
      return;

    try {
      scanner = adapter.getBluetoothLeScanner();
      if (scanner != null)
        scanner.startScan(this);
    } catch (Exception e) {
      Log.e(TAG, "Bluetooth LE scan failed", e);
      scanner = null;
    }
  }

  private final PermissionManager.PermissionHandler leScanPermissionHandler =
    new PermissionManager.PermissionHandler() {
      @Override
      public void onRequestPermissionsResult(boolean granted) {
        if (granted)
          /* try again */
          startLeScan();
      }
    };

  public synchronized void addDetectDeviceListener(DetectDeviceListener l) {
    detectListeners.add(l);

    if (requestConnectPermission(bondedPermissionHandler)) {
      try {
        Set<BluetoothDevice> devices = adapter.getBondedDevices();
        if (devices != null)
          submitBondedDevices(devices, l);
      } catch (SecurityException e) {
        // we don't have permission.BLUETOOTH_CONNECT
        Log.e(TAG, "Cannot list bonded Bluetooth devices", e);
      }
    }

    if (hasLe) {
      if (requestScanPermission(leScanPermissionHandler))
        startLeScan();
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

    // TODO wait for permission to be granted
    requestConnectPermission(null);

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

    // TODO wait for permission to be granted
    requestConnectPermission(null);

    Log.d(TAG, String.format("Bluetooth device \"%s\" is a LE device, trying to connect using GATT...",
                             getDisplayString(device)));
    return new HM10Port(context, device);
  }

  public AndroidPort connect(String address)
    throws IOException {
    BluetoothDevice device = adapter.getRemoteDevice(address);
    if (device == null)
      throw new IOException("Bluetooth device not found");

    // TODO wait for permission to be granted
    requestConnectPermission(null);

    BluetoothSocket socket =
      device.createRfcommSocketToServiceRecord(THE_UUID);
    return new BluetoothClientPort(socket);
  }

  public AndroidPort createServer() throws IOException {
    // TODO wait for permission to be granted
    requestConnectPermission(null);

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
                         device.getAddress(), getName(device),
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
