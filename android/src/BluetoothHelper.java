// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.UUID;
import java.util.Set;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.LinkedList;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.io.IOException;

import android.os.ParcelUuid;
import android.util.Log;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.BluetoothSocket;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanRecord;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanSettings;
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

  /**
   * True when the bonded device caches the PPG engine-sensor GATT
   * service.  Used to hide Engine Type for heart-rate BLE sensors.
   */
  public boolean hasEngineSensors(String address) {
    try {
      BluetoothDevice device = adapter.getRemoteDevice(address);
      ParcelUuid[] uuids = device.getUuids();
      if (uuids == null)
        return false;
      for (ParcelUuid puuid : uuids)
        if (BluetoothUuids.ENGINE_SENSORS_SERVICE.equals(puuid.getUuid()))
          return true;
    } catch (IllegalArgumentException | SecurityException e) {
      return false;
    }
    return false;
  }

  /**
   * BlueFly Vario BLE uses the Microchip/ISSC transparent UART, but
   * bonded devices often have no cached service UUIDs.  The advertised
   * name is enough to offer them as a BLE serial port.
   */
  private static boolean isBlueFlyName(String name) {
    return name != null && name.regionMatches(true, 0, "BlueFly", 0, 7);
  }

  private static long getBondedLeFeatures(BluetoothDevice device, String name) {
    long features = 0;

    if (isBlueFlyName(name))
      features |= DetectDeviceListener.FEATURE_BLE_SERIAL;

    try {
      ParcelUuid[] uuids = device.getUuids();
      if (uuids != null)
        features |= getFeatures(Arrays.asList(uuids));
    } catch (SecurityException e) {
      /* BLUETOOTH_CONNECT may still be missing on Android 12+ */
    }

    return features;
  }

  private static void submitBondedDevices(Collection<BluetoothDevice> devices,
                                          DetectDeviceListener l) {
    for (BluetoothDevice device : devices) {
      final int hwType = device.getType();
      final String name = getName(device);
      final String address = device.getAddress();

      /* Dual-mode modules (e.g. BlueFly RN4678) are not DEVICE_TYPE_LE
         only; still offer the classic SPP entry when Android reports
         classic or dual. */
      if (hwType != BluetoothDevice.DEVICE_TYPE_LE)
        l.onDeviceDetected(DetectDeviceListener.TYPE_BLUETOOTH_CLASSIC,
                           address, name, 0);

      if (hwType == BluetoothDevice.DEVICE_TYPE_LE ||
          hwType == BluetoothDevice.DEVICE_TYPE_DUAL)
        l.onDeviceDetected(DetectDeviceListener.TYPE_BLUETOOTH_LE,
                           address, name,
                           getBondedLeFeatures(device, name));
    }
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
    /**
     * Build ScanFilters for SERVICE UUIDs, only
     * devices providing the services will be found
     * by the scanner. So later onServicesDiscovered() will
     * just trigger for supported services.
     * */ 
    List<ScanFilter> filters = new ArrayList<>();
    for (UUID uuid : BluetoothUuids.getAllServiceUuids()) {
        ScanFilter filter = new ScanFilter.Builder()
                .setServiceUuid(new ParcelUuid(uuid))
                .build();
        filters.add(filter);
        Log.d(TAG, "Filter for known BLE services: " + uuid.toString());
    }

    // Create ScanSettings, quick scan for the supported services.
    ScanSettings settings = new ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build();

    // Start scanning with filters and settings.
    try {
      scanner = adapter.getBluetoothLeScanner();
      if (scanner != null)
        scanner.startScan(filters, settings, this);
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

  /**
   * Xiaomi (and other DUAL) bands pair as a public BR/EDR address
   * but talk GATT on a random LE address.  Prefer that exact address;
   * use the advertised name only when it matches one live device.
   */
  private static boolean addressEquals(BluetoothDevice other, String address) {
    return other != null && address != null &&
      address.equalsIgnoreCase(other.getAddress());
  }

  private static boolean nameEquals(BluetoothDevice other, String name) {
    if (other == null || name == null || name.isEmpty())
      return false;
    try {
      return name.equals(getName(other));
    } catch (SecurityException e) {
      return false;
    }
  }

  private static BluetoothDevice uniqueNameMatch(Iterable<BluetoothDevice> devices,
                                                 String name) {
    BluetoothDevice unique = null;
    for (BluetoothDevice d : devices) {
      if (!nameEquals(d, name))
        continue;
      if (unique == null)
        unique = d;
      else if (!unique.getAddress().equalsIgnoreCase(d.getAddress()))
        return null;
    }
    return unique;
  }

  private BluetoothDevice findConnectedLeDevice(BluetoothDevice configured) {
    BluetoothManager manager = (BluetoothManager)
      context.getSystemService(Context.BLUETOOTH_SERVICE);
    if (manager == null)
      return null;

    final String address = configured.getAddress();
    final String name = getName(configured);
    final int[] profiles = {
      BluetoothProfile.GATT, BluetoothProfile.GATT_SERVER
    };
    final int[] states = {
      BluetoothProfile.STATE_CONNECTED,
      BluetoothProfile.STATE_CONNECTING
    };
    final List<BluetoothDevice> named = new ArrayList<BluetoothDevice>();

    try {
      for (int profile : profiles)
        for (BluetoothDevice d :
               manager.getDevicesMatchingConnectionStates(profile, states)) {
          if (addressEquals(d, address))
            return d;
          if (nameEquals(d, name))
            named.add(d);
        }
    } catch (SecurityException e) {
      return null;
    }
    return uniqueNameMatch(named, name);
  }

  private BluetoothDevice scanForLeDevice(final BluetoothDevice configured,
                                          int timeout_ms) {
    final BluetoothLeScanner le_scanner = adapter.getBluetoothLeScanner();
    if (le_scanner == null)
      return null;

    final String address = configured.getAddress();
    final String name = getName(configured);
    final BluetoothDevice[] by_address = new BluetoothDevice[1];
    final Map<String, BluetoothDevice> by_name =
      new LinkedHashMap<String, BluetoothDevice>();
    final CountDownLatch done = new CountDownLatch(1);
    final ScanCallback cb = new ScanCallback() {
      @Override
      public void onScanResult(int callbackType, ScanResult result) {
        BluetoothDevice d = result.getDevice();
        if (addressEquals(d, address)) {
          by_address[0] = d;
          done.countDown();
          return;
        }
        if (nameEquals(d, name))
          synchronized (by_name) {
            by_name.put(d.getAddress().toUpperCase(), d);
          }
      }
    };

    List<ScanFilter> filters = new ArrayList<>();
    try {
      filters.add(new ScanFilter.Builder().setDeviceAddress(address).build());
    } catch (IllegalArgumentException e) {
      /* keep name filter */
    }
    if (name != null && !name.isEmpty())
      filters.add(new ScanFilter.Builder().setDeviceName(name).build());

    ScanSettings settings = new ScanSettings.Builder()
      .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
      .build();

    try {
      le_scanner.startScan(filters, settings, cb);
      done.await(timeout_ms, TimeUnit.MILLISECONDS);
    } catch (Exception e) {
      Log.e(TAG, "BLE sensor scan failed", e);
    } finally {
      try {
        le_scanner.stopScan(cb);
      } catch (Exception e) {
      }
    }

    if (by_address[0] != null)
      return by_address[0];

    synchronized (by_name) {
      if (by_name.size() == 1)
        return by_name.values().iterator().next();
    }
    return null;
  }

  public BluetoothSensor connectSensor(String address, SensorListener listener)
    throws IOException
  {
    if (!hasLe)
      throw new IOException("No Bluetooth LE support");

    // TODO wait for permission to be granted
    requestConnectPermission(null);
    requestScanPermission(null);

    BluetoothDevice configured = adapter.getRemoteDevice(address);
    if (configured == null)
      throw new IOException("Bluetooth device not found");

    BluetoothDevice device = findConnectedLeDevice(configured);
    boolean auto_connect = true;
    if (device != null) {
      auto_connect = false;
      Log.i(TAG, "BLE sensor " + address + " using connected " +
            device.getAddress());
    } else {
      device = scanForLeDevice(configured, 4000);
      if (device != null) {
        auto_connect = false;
        Log.i(TAG, "BLE sensor " + address + " using advertised " +
              device.getAddress());
      } else {
        device = configured;
        Log.i(TAG, "BLE sensor " + address + " using configured address");
      }
    }

    return new BluetoothSensor(context, device, listener, auto_connect);
  }

  public AndroidPort connectBleSerial(String address)
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
    return BleSerialPort.create(context, device);
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
        features |= DetectDeviceListener.FEATURE_BLE_SERIAL;
      else if (BluetoothUuids.NORDIC_UART_SERVICE.equals(uuid))
        features |= DetectDeviceListener.FEATURE_BLE_SERIAL;
      else if (BluetoothUuids.ISSC_UART_SERVICE.equals(uuid))
        features |= DetectDeviceListener.FEATURE_BLE_SERIAL;
      else if (BluetoothUuids.HEART_RATE_SERVICE.equals(uuid))
        features |= DetectDeviceListener.FEATURE_HEART_RATE;
      else if (BluetoothUuids.PULSE_OXIMETER_SERVICE.equals(uuid))
        features |= DetectDeviceListener.FEATURE_PULSE_OXIMETER;
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
    String name = getName(device);
    long features = getFeatures(result);
    if (isBlueFlyName(name))
      features |= DetectDeviceListener.FEATURE_BLE_SERIAL;

    for (DetectDeviceListener l : detectListeners)
      l.onDeviceDetected(DetectDeviceListener.TYPE_BLUETOOTH_LE,
                         device.getAddress(), name,
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
