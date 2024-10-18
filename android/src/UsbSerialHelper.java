// SPDX-License-Identifier: GPL-2.0-only
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Arrays;
import java.util.Collection;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Iterator;
import java.io.IOException;

import android.annotation.TargetApi;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.hardware.usb.UsbConstants;
import android.os.Build;
import android.util.Log;

import com.felhr.usbserial.UsbSerialDevice;

@TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1)
public final class UsbSerialHelper extends BroadcastReceiver {

  private static final String TAG = "UsbSerialHelper";
  private static final String ACTION_USB_PERMISSION = "org.xcsoar.otg.action.USB_PERMISSION";

  private final Context context;
  private final UsbManager usbmanager;

  private final Collection<UsbDeviceInterface> interfaces = new LinkedList<>();

  private final Collection<DetectDeviceListener> detectListeners =
    new LinkedList<DetectDeviceListener>();

  // keep this list in sync with res/xml/usb_device_filter.xml
  private static final long[] supported_ids = createTable(
    createDevice(0x16D0, 0x0BA9), // GPSBip
    createDevice(0x0403, 0x6015), // Digifly AIR (FT X-Series)
    createDevice(0x0483, 0x5740), // SoftRF Dongle
    createDevice(0x239A, 0x8029), // SoftRF Badge
    createDevice(0x2341, 0x0069), // SoftRF Academy
    createDevice(0x2341, 0x1002), // SoftRF Academy
    createDevice(0x2341, 0x804d), // SoftRF Academy
    createDevice(0x239A, 0xCAFE), // SoftRF Academy
    createDevice(0x1d50, 0x6089), // SoftRF ES
    createDevice(0x2e8a, 0x000a), // SoftRF Lego
    createDevice(0x2e8a, 0xf00a), // SoftRF Lego
    createDevice(0x15ba, 0x0044), // SoftRF Balkan
    createDevice(0x303a, 0x1001), // SoftRF Eco
    createDevice(0x303a, 0x8133), // SoftRF Prime Mk3
    createDevice(0x303a, 0x818f), // SoftRF Ham
    createDevice(0x303a, 0x81a0), // SoftRF Midi
    createDevice(0x303a, 0x820a), // SoftRF Ink
    createDevice(0x2886, 0x0057), // SoftRF Card

    createDevice(0x0403, 0x6001), // FT232AM, FT232BM, FT232R FT245R,
    createDevice(0x0403, 0x6010), // FT2232D, FT2232H
    createDevice(0x0403, 0x6011), // FT4232H
    createDevice(0x0403, 0x6014), // FT232H

    createDevice(0x10C4, 0xEA60), // CP210x
    createDevice(0x1A86, 0x55D4), // CH9102

    createDevice(0x067B, 0x2303), // PL2303
    createDevice(0x1546, 0x01A7)  // U-BLOX 7 USB GPS
  );

  private static String makeDeviceId(UsbDevice device,
                                     boolean withSerialNumber) {
    String id = String.format("%04X:%04X",
                              device.getVendorId(), device.getProductId());

    if (withSerialNumber) {
      String serialNumber = getSerialNumber(device);
      if (serialNumber != null)
        id += "[" + serialNumber + "]";
    }

    return id;
  }

  private static String makePortId(UsbDevice device, int iface,
                                   boolean withSerialNumber) {
    String id = makeDeviceId(device, withSerialNumber);

    if (iface > 0)
      /* a secondary interface on the same device: append the
         interface index */
      id += "#" + iface;

    return id;
  }

  final class UsbDeviceInterface {
    public final UsbDevice device;
    public final int iface;
    public final String id;

    /**
     * The id without the serial number.  This is used to match
     * old-style device ids from old XCSoar profiles.
     */
    public final String oldId;

    /**
     * If not null, then this port instance is currently being used by
     * native code.  It may be waiting for permission from the
     * UsbManager, or it may already be open.
     */
    public UsbSerialPort port;

    public UsbDeviceInterface(UsbDevice dev_,int iface_) {
      device = dev_;
      iface = iface_;
      id = makePortId(device, iface, true);
      oldId = makePortId(device, iface, false);
    }

    public boolean isDevice(UsbDevice otherDevice) {
      return isSameDevice(device, otherDevice);
    }

    public boolean isId(String otherId) {
      /* compare both id and oldId, because the XCSoar profile setting
         may be older than XCSoar 7.25 and thus may not have the
         serial number */
      return otherId.contentEquals(id) || otherId.contentEquals(oldId);
    }

    public String getDisplayName() {
      String name = device.getProductName();
      if (name == null)
        name = id;

      String manufacturer = device.getManufacturerName();
      if (manufacturer != null)
        name += " (" + manufacturer + ")";

      String serialNumber = getSerialNumber(device);
      if (serialNumber != null) {
        name += " [" + serialNumber + "]";
      }

      // add interface number to name only when more than one interface
      if (device.getInterfaceCount() > 1)
        name += "#" + (iface + 1);

      return name;
    }

    public synchronized AndroidPort open(int baud) throws IOException {
      if (port != null)
        throw new IOException("Port already occupied");

      port = new UsbSerialPort(this, baud);
      if (usbmanager.hasPermission(device)) {
        port.open(usbmanager, device, iface);
      } else {
        requestPermission(device);
      }

      return port;
    }

    public synchronized void permissionGranted() {
      if (port != null && !port.isOpen())
        port.open(usbmanager, device, iface);
    }

    /**
     * Called by UsbSerialPort::close().
     */
    public synchronized void portClosed(UsbSerialPort _port) {
      if (_port == port)
        port = null;
    }

    public synchronized void onConnect(UsbDevice newDevice) {
      // TODO implement
    }

    public synchronized void onDisconnect() {
      if (port != null) {
        port.onDisconnect();
        port = null;
      }
    }
  }

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

  private static boolean isSupported(UsbDevice device) {
    return UsbSerialDevice.isSupported(device) &&
      exists(supported_ids, device.getVendorId(), device.getProductId());
  }

  private static boolean isSameDeviceName(UsbDevice a, UsbDevice b) {
    return a.getDeviceName().equals(b.getDeviceName());
  }

  private static String getSerialNumber(UsbDevice device) {
    try {
      return device.getSerialNumber();
    } catch (SecurityException e) {
      /* should not happen, because we already requested permission,
         but better be safe than sorry */
      return null;
    }
  }

  private static boolean isSameSerialNumber(UsbDevice a, UsbDevice b) {
    String as = getSerialNumber(a);
    String bs = getSerialNumber(b);

    return as == null || bs == null || as.contentEquals(bs);
  }

  private static boolean isSameDevice(UsbDevice a, UsbDevice b) {
    return isSameDeviceName(a, b) && isSameSerialNumber(a, b);
  }

  @Override
  public synchronized void onReceive(Context context, Intent intent) {
    synchronized (this) {
      String action = intent.getAction();
      if (!intent.hasExtra(UsbManager.EXTRA_DEVICE))
        return;

      UsbDevice device = (UsbDevice) intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
      if (device == null)
        return;

      if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
        if (usbmanager.hasPermission(device))
          addAvailable(device);
        else
          /* always request permission which is needed to read the
             serial number */
          requestPermission(device);
      } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
        removeAvailable(device);
      } else if (ACTION_USB_PERMISSION.equals(action)) {
        if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
          Log.d(TAG, "permission granted for device " + device.getDeviceName());

          //Iterate through list of Pending connections. For each entry matching with granted device, open port and remove from list
          boolean found = false;
          for (UsbDeviceInterface i : interfaces) {
            if (i.isDevice(device)) {
              found = true;
              i.permissionGranted();
            }
          }

          if (!found)
            addAvailable(device);
        }
      }
    }
  }

  private synchronized void addAvailable(UsbDevice device) {
    if (!isSupported(device))
      return;

    final int n = device.getInterfaceCount();
    final UsbDeviceInterface[] existing = new UsbDeviceInterface[n];

    /* just in case there are old versions of this device in the list
       (shouldn't happen, but who knows), update them */
    for (Iterator<UsbDeviceInterface> iter = interfaces.iterator();
         iter.hasNext();) {
      UsbDeviceInterface iface = iter.next();
      if (!iface.isDevice(device))
        continue;

      if (iface.iface < n) {
        /* update the existing instance */
        existing[iface.iface] = iface;
        iface.onConnect(device);
      } else {
        /* this interface index doesn't exist anymore, remove it */
        iface.onDisconnect();
        iter.remove();
      }
    }

    for (int iface=0; iface < n; iface++) {
      if (existing[iface] != null)
        /* still exists */
        continue;

      int iclass = device.getInterface(iface).getInterfaceClass();
      if (iclass == UsbConstants.USB_CLASS_VENDOR_SPEC || iclass == UsbConstants.USB_CLASS_CDC_DATA) {
        UsbDeviceInterface deviface = new UsbDeviceInterface(device, iface);
        interfaces.add(deviface);
        broadcastDetectedDeviceInterface(deviface);
      }
    }
  }

  private synchronized UsbDeviceInterface getAvailable(String id) {
    for (UsbDeviceInterface i : interfaces)
      if (i.isId(id))
        return i;

    return null;
  }

  private synchronized void removeAvailable(UsbDevice removeddevice) {
    Iterator<UsbDeviceInterface> iter = interfaces.iterator();
    while (iter.hasNext()) {
      UsbDeviceInterface iface = iter.next();
      if (iface.isDevice(removeddevice)) {
        iface.onDisconnect();
        iter.remove();
      }
    }
  }

  private UsbSerialHelper(Context context) throws IOException {
    this.context = context;

    usbmanager = (UsbManager) context.getSystemService(Context.USB_SERVICE);
    if (usbmanager == null)
      throw new IOException("No USB service");

    HashMap<String, UsbDevice> devices = usbmanager.getDeviceList();
    for (Map.Entry<String, UsbDevice> entry : devices.entrySet()) {
      addAvailable(entry.getValue());
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

  private void requestPermission(UsbDevice device) {
    PendingIntent pi =
      PendingIntent.getBroadcast(context, 0,
                                 new Intent(UsbSerialHelper.ACTION_USB_PERMISSION),
                                 PendingIntent.FLAG_IMMUTABLE);

    usbmanager.requestPermission(device, pi);
  }

  private synchronized AndroidPort connect(String id, int baud)
    throws IOException
  {
    UsbDeviceInterface deviface = getAvailable(id);
    if (deviface == null)
      throw new IOException("USB serial device not found");

    return deviface.open(baud);
  }

  private synchronized void broadcastDetectedDeviceInterface(UsbDeviceInterface deviface) {
    if (detectListeners.isEmpty())
      return;

    final String name = deviface.getDisplayName();

    for (DetectDeviceListener l : detectListeners)
      l.onDeviceDetected(DetectDeviceListener.TYPE_USB_SERIAL, deviface.id, name, 0);
  }

  public synchronized void addDetectDeviceListener(DetectDeviceListener l) {
    detectListeners.add(l);

    for (UsbDeviceInterface iface : interfaces)
      l.onDeviceDetected(DetectDeviceListener.TYPE_USB_SERIAL, iface.id,
                         iface.getDisplayName(), 0);
  }

  public synchronized void removeDetectDeviceListener(DetectDeviceListener l) {
    detectListeners.remove(l);
  }
}
