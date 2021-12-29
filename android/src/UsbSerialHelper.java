/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; version 2
  of the License.

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
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.io.IOException;

import android.annotation.TargetApi;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.util.Log;

import com.felhr.usbserial.UsbSerialDevice;

@TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1)
public class UsbSerialHelper extends BroadcastReceiver {

  private static final String TAG = "UsbSerialHelper";
  public static final String ACTION_USB_PERMISSION = "org.xcsoar.otg.action.USB_PERMISSION";

  private final Context context;
  private final UsbManager usbmanager;

  private HashMap<String, UsbDevice> _AvailableDevices = new HashMap<>();
  private HashMap<UsbDevice, UsbSerialPort> _PendingConnection = new HashMap<>();

  private final Collection<DetectDeviceListener> detectListeners =
    new LinkedList<DetectDeviceListener>();

  private static final long[] supported_ids = createTable(
    createDevice(0x16D0, 0x0BA9), // GPSBip
    createDevice(0x0403, 0x6015), // Digifly AIR (FT X-Series)
    createDevice(0x0483, 0x5740), // SoftRF Dongle
    createDevice(0x239A, 0x8029), // SoftRF Badge

    createDevice(0x0403, 0x6001), // FT232AM, FT232BM, FT232R FT245R,
    createDevice(0x0403, 0x6010), // FT2232D, FT2232H
    createDevice(0x0403, 0x6011), // FT4232H
    createDevice(0x0403, 0x6014), // FT232H

    createDevice(0x10C4, 0xEA60), // CP210x
    createDevice(0x1A86, 0x55D4), // CH9102

    createDevice(0x067B, 0x2303) // PL2303
  );

  static long createDevice(int vendorId, int productId) {
    return ((long) vendorId) << 32 | (productId & 0xFFFF_FFFFL);
  }

  static long[] createTable(long... entries) {
    Arrays.sort(entries);
    return entries;
  }

  static boolean exists(long[] devices, int vendorId, int productId) {
    return Arrays.binarySearch(devices, createDevice(vendorId, productId)) >= 0;
  }

  @Override
  public synchronized void onReceive(Context context, Intent intent) {
    synchronized (this) {
      String action = intent.getAction();
      if (!intent.hasExtra(UsbManager.EXTRA_DEVICE)) {
        return;
      }

      UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
      if (device == null) {
        return;
      }
      if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
        AddAvailable(device);
      } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
        RemoveAvailable(device);
      } else if (ACTION_USB_PERMISSION.equals(action)) {
        if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
          Log.d(TAG, "permission granted for device " + device);

          UsbSerialPort port = _PendingConnection.get(device);
          _PendingConnection.remove(device);
          if (port != null) {
            port.open(usbmanager);
          }
        }
      }
    }
  }

  private synchronized void AddAvailable(UsbDevice device) {
    if (UsbSerialDevice.isSupported(device)) {
      int vid = device.getVendorId();
      int pid = device.getProductId();

      if (exists(supported_ids, vid, pid)) {
        Log.v(TAG, "UsbDevice Found : " + device);
        _AvailableDevices.put(device.getDeviceName(), device);
        broadcastDetectedDevice(device);
      } else {
        Log.v(TAG, "Unsupported UsbDevice : " + device);
      }
    }
  }

  private synchronized UsbDevice GetAvailable(String name) {
    for (Map.Entry<String, UsbDevice> entry : _AvailableDevices.entrySet()) {
      if(name.contentEquals(getDeviceId(entry.getValue()))) {
        return entry.getValue();
      }
    }
    return null;
  }

  private synchronized void RemoveAvailable(UsbDevice device) {
    Log.v(TAG,"UsbDevice disconnected : " + device);
    _AvailableDevices.remove(device.getDeviceName());
  }

  private UsbSerialHelper(Context context) throws IOException {
    this.context = context;

    usbmanager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
    if (usbmanager == null)
      throw new IOException("No USB service");

    HashMap<String, UsbDevice> devices = usbmanager.getDeviceList();
    for (Map.Entry<String, UsbDevice> entry : devices.entrySet()) {
      AddAvailable(entry.getValue());
    }

    registerReceiver();
  }

  private void close() {
    unregisterReceiver();
  }

  private void registerReceiver() {
    IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
    filter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
    filter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
    context.registerReceiver(this, filter);
  }

  private void unregisterReceiver() {
    context.unregisterReceiver(this);
  }

  private synchronized AndroidPort connect(String name, int baud)
    throws IOException
  {
    UsbDevice device = GetAvailable(name);
    if (device == null)
      throw new IOException("USB serial device not found");

    UsbSerialPort port = new UsbSerialPort(device,baud);
    if (usbmanager.hasPermission(device)) {
      port.open(usbmanager);
    } else {
      _PendingConnection.put(device, port);
      PendingIntent pi = PendingIntent.getBroadcast(context, 0, new Intent(UsbSerialHelper.ACTION_USB_PERMISSION), 0);
      usbmanager.requestPermission(device, pi);
    }

    return port;
  }

  private synchronized void broadcastDetectedDevice(UsbDevice device) {
    if (detectListeners.isEmpty())
      return;

    final String id = getDeviceId(device);
    final String name = getDeviceDisplayName(device);

    for (DetectDeviceListener l : detectListeners)
      l.onDeviceDetected(DetectDeviceListener.TYPE_USB_SERIAL, id, name, 0);
  }

  public synchronized void addDetectDeviceListener(DetectDeviceListener l) {
    detectListeners.add(l);

    for (Map.Entry<String, UsbDevice> entry : _AvailableDevices.entrySet())
      broadcastDetectedDevice(entry.getValue());
  }

  public synchronized void removeDetectDeviceListener(DetectDeviceListener l) {
    detectListeners.remove(l);
  }

  static String getDeviceId(UsbDevice device) {
    return String.format("%04X:%04X", device.getVendorId(), device.getProductId());
  }

  static String getDeviceDisplayName(UsbDevice device) {
    String name = device.getProductName();
    if (name == null)
      name = getDeviceId(device);

    String manufacturer = device.getManufacturerName();
    if (manufacturer != null)
      name += " (" + manufacturer + ")";

    return name;
  }
}
