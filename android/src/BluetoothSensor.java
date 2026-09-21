// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import java.util.Queue;
import java.util.UUID;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
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
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

/**
 * Read Bluetooth LE sensor values and report them to a
 * #SensorListener.
 */
public final class BluetoothSensor
  extends BluetoothGattCallback
  implements AndroidSensor
{
  private static final String TAG = "XCSoar";

  private final SensorListener listener;
  private final SafeDestruct safeDestruct = new SafeDestruct();

  /** kept for reconnecting after a failed connection attempt */
  private final Context context;
  private final BluetoothDevice device;
  private final boolean auto_connect;
  private volatile boolean shutdown = false;

  /**
   * Assigned on the main thread, read on the Binder thread that
   * delivers the GATT callbacks, so the two have to agree on what
   * they see.
   */
  private volatile BluetoothGatt gatt;

  /**
   * The client retryConnect() has closed.  It is cleared from #gatt
   * before the replacement exists, so without remembering it here a
   * callback arriving in that gap would look like the constructor's
   * first one.
   */
  private volatile BluetoothGatt retired;

  private int state = STATE_LIMBO;

  /**
   * Android drops the first connection attempt to a BLE device often
   * enough that treating it as fatal is wrong: reporting a failure
   * makes DeviceDescriptor::OnSysTicker() close the whole device and
   * reopen it seconds later, which the pilot sees as an error message
   * followed by a connection that works anyway.  Retry in place
   * instead, and only give up once the device has had its chances.
   */
  private static final int MAX_CONNECT_RETRIES = 2;
  private int connectRetries = 0;

  /**
   * Has this object ever reached STATE_CONNECTED?  A drop before that
   * is a failed attempt and worth retrying; one after it is the
   * device going away, which is not.
   */
  private boolean everConnected = false;

  private BluetoothGattCharacteristic currentEnableNotification;
  private final Queue<BluetoothGattCharacteristic> enableNotificationQueue =
    new LinkedList<BluetoothGattCharacteristic>();

  private boolean haveFlytecMovement = false;
  private double flytecGroundSpeed, flytecTrack;
  private int flytecSatellites = 0;

  public BluetoothSensor(final Context context, final BluetoothDevice device,
                         SensorListener listener)
    throws IOException
  {
    this(context, device, listener, true);
  }

  public BluetoothSensor(final Context context, final BluetoothDevice device,
                         SensorListener listener, final boolean autoConnect)
    throws IOException
  {
    this.listener = listener;
    this.context = context;
    this.device = device;
    this.auto_connect = autoConnect;

    /**
     * Run GATT connect on the main thread on API 23+: some Android
     * versions close the client if connectGatt() is issued from a
     * worker.  Wait for that posted work so this constructor does not
     * throw "GATT connect failed" while gatt is still null.
     */
    if (Build.VERSION.SDK_INT >= 23 &&
        Looper.myLooper() != Looper.getMainLooper()) {
      final Handler handler = new Handler(Looper.getMainLooper());
      final AtomicBoolean abandoned = new AtomicBoolean(false);
      final CountDownLatch done = new CountDownLatch(1);
      final IOException[] error = new IOException[1];
      final Runnable start = new Runnable() {
        @Override
        public void run() {
          if (abandoned.get())
            return;
          try {
            connectGatt();
          } catch (IOException e) {
            error[0] = e;
          } finally {
            if (abandoned.get()) {
              if (gatt != null) {
                gatt.close();
                gatt = null;
              }
              return;
            }
            done.countDown();
          }
        }
      };
      handler.post(start);
      try {
        if (!done.await(5, TimeUnit.SECONDS)) {
          abandonGattConnect(handler, start, abandoned);
          throw new IOException("Bluetooth GATT connect timed out");
        }
      } catch (InterruptedException e) {
        abandonGattConnect(handler, start, abandoned);
        Thread.currentThread().interrupt();
        throw new IOException("Bluetooth GATT connect interrupted", e);
      }
      if (error[0] != null)
        throw error[0];
    } else {
      connectGatt();
    }
  }

  /**
   * LE transport, matching BleSerialPort.  autoConnect waits for the
   * next advertisement; a live/scanned device uses a direct connect.
   */
  private void connectGatt()
    throws IOException
  {
    try {
      if (Build.VERSION.SDK_INT >= 23)
        gatt = device.connectGatt(context, auto_connect, this,
                                  BluetoothDevice.TRANSPORT_LE);
      else
        gatt = device.connectGatt(context, auto_connect, this);
    } catch (SecurityException e) {
      throw new IOException("Bluetooth GATT connect not permitted", e);
    }
    if (gatt == null)
      throw new IOException("Bluetooth GATT connect failed");
  }

  /**
   * Drop a connectGatt posted to the main looper if this constructor
   * fails, so a late callback cannot keep a GATT client open.
   */
  private void abandonGattConnect(Handler handler, Runnable start,
                                  AtomicBoolean abandoned) {
    abandoned.set(true);
    handler.removeCallbacks(start);
    handler.post(new Runnable() {
      @Override
      public void run() {
        if (gatt != null) {
          gatt.close();
          gatt = null;
        }
      }
    });
  }

  @Override
  public void close() {
    shutdown = true;
    safeDestruct.beginShutdown();
    if (gatt != null)
      gatt.close();
    safeDestruct.finishShutdown();
  }

  @Override
  public int getState() {
    return state;
  }

  private void setStateSafe(int _state) {
    if (_state == state)
      return;

    state = _state;

    if (safeDestruct.increment()) {
      try {
        listener.onSensorStateChanged();
      } finally {
        safeDestruct.decrement();
      }
    }
  }

  private void submitError(String msg) {
    haveFlytecMovement = false;
    flytecSatellites = 0;
    state = STATE_FAILED;

    if (safeDestruct.increment()) {
      try {
        listener.onSensorError(msg);
      } finally {
        safeDestruct.decrement();
      }
    }
  }

  private boolean doEnableNotification(BluetoothGattCharacteristic c) {
    BluetoothGattDescriptor d = c.getDescriptor(BluetoothUuids.CLIENT_CHARACTERISTIC_CONFIGURATION);
    if (d == null)
      return false;

    gatt.setCharacteristicNotification(c, true);
    d.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
    return gatt.writeDescriptor(d);
  }

  private void enableNotification(BluetoothGattCharacteristic c) {
    synchronized(enableNotificationQueue) {
      if (currentEnableNotification == null) {
        currentEnableNotification = c;
        if (!doEnableNotification(c))
          currentEnableNotification = null;
      } else
        enableNotificationQueue.add(c);
    }
  }

  /**
   * Data in the characteristic has little endian byteorder.
   * Lowest bit of flags indicates valid ignitions_per_sec reading.
   * 0 Kelvin indicates invalid temperatures e.g 
   * no temperature sensor present.
  */
  private void engineSensorDataToListeners(BluetoothGattCharacteristic c) {
    final int flags = c.getIntValue(c.FORMAT_UINT8, 0);
    final int cht_temp = c.getIntValue(c.FORMAT_UINT16, 1);
    final int egt_temp = c.getIntValue(c.FORMAT_UINT16, 3);
    final int outside_air_temperature = c.getIntValue(c.FORMAT_UINT16, 5);

    if(outside_air_temperature != 0)
      listener.onTemperature(outside_air_temperature);

    final int pressure = c.getIntValue(c.FORMAT_UINT32, 7);

    // Just guessing the sensor_noise_variance.
    if(pressure != 0)
      listener.onBarometricPressureSensor(pressure / 100.0f, 0.01f);

    final int ignitions_per_second = c.getIntValue(c.FORMAT_UINT16, 11);
    listener.onEngineSensors(cht_temp != 0 ? true : false,
                             cht_temp,
                             egt_temp != 0 ? true : false,
                             egt_temp,
                             (flags&0x01) == 0x01 ? true : false,
                             ignitions_per_second);
  }

  private void readHeartRateMeasurement(BluetoothGattCharacteristic c) {
    int offset = 0;
    final int flags = c.getIntValue(c.FORMAT_UINT8, offset);
    ++offset;

    final boolean bpm16 = (flags & 0x1) != 0;

    final int bpm = bpm16
      ? c.getIntValue(c.FORMAT_UINT16, offset)
      : c.getIntValue(c.FORMAT_UINT8, offset);

    listener.onHeartRateSensor(bpm);
  }

  static long toUnsignedLong(int x) {
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N)
      // Android 7 "Nougat" supports Java 8
      return Integer.toUnsignedLong(x);

    // Reimplement for older Android versions
    long l = x;
    if (l < 0)
      l += (1L << 32);
    return l;
  }

  @Override
  public synchronized void onCharacteristicChanged(BluetoothGatt gatt,
                                                   BluetoothGattCharacteristic c) {
    if (!safeDestruct.increment())
      return;

    try {
      if (BluetoothUuids.HEART_RATE_MEASUREMENT_CHARACTERISTIC.equals(c.getUuid())) {
        readHeartRateMeasurement(c);
      }

      if (BluetoothUuids.ENGINE_SENSORS_CHARACTERISTIC.equals(c.getUuid())) {
        engineSensorDataToListeners(c);
      }

      if (BluetoothUuids.FLYTEC_SENSBOX_NAVIGATION_SENSOR_CHARACTERISTIC.equals(c.getUuid())) {        
        /* protocol documentation:
            https://github.com/flytec/SensBoxLib_iOS/blob/master/_SensBox%20Documentation/SensorBox%20BLE%20Protocol.pdf */
        final int gps_status = c.getIntValue(c.FORMAT_UINT8, 18) & 0x7;
        final boolean hasAltitude = (gps_status == 2 || gps_status == 4);

        final long time = 1000 *
          toUnsignedLong(c.getIntValue(c.FORMAT_UINT32, 0));

        listener.onLocationSensor(time,
                                  flytecSatellites,
                                  c.getIntValue(c.FORMAT_SINT32, 8) / 10000000.,
                                  c.getIntValue(c.FORMAT_SINT32, 4) / 10000000.,
                                  hasAltitude, true,
                                  c.getIntValue(c.FORMAT_SINT16, 12),
                                  haveFlytecMovement, flytecTrack,
                                  haveFlytecMovement, flytecGroundSpeed,
                                  false, 0);

        listener.onPressureAltitudeSensor(c.getIntValue(c.FORMAT_SINT16, 14));
      } else if (BluetoothUuids.FLYTEC_SENSBOX_MOVEMENT_SENSOR_CHARACTERISTIC.equals(c.getUuid())) {
        flytecGroundSpeed = c.getIntValue(c.FORMAT_SINT16, 6) / 10.;
        flytecTrack = c.getIntValue(c.FORMAT_SINT16, 8) / 10.;

        listener.onVarioSensor(c.getIntValue(c.FORMAT_SINT16, 4) / 100.f);
        listener.onAccelerationSensor1(c.getIntValue(c.FORMAT_UINT16, 16) / 10.);

        haveFlytecMovement = true;
      } else if (BluetoothUuids.FLYTEC_SENSBOX_SECOND_GPS_CHARACTERISTIC.equals(c.getUuid())) {
        flytecSatellites = c.getIntValue(c.FORMAT_UINT8, 6);
      } else if (BluetoothUuids.FLYTEC_SENSBOX_SYSTEM_CHARACTERISTIC.equals(c.getUuid())) {
        listener.onBatteryPercent(c.getIntValue(c.FORMAT_UINT8, 4));

        final double CELSIUS_OFFSET = 273.15;
        double temperatureCelsius = c.getIntValue(c.FORMAT_SINT16, 6) / 10.;
        listener.onTemperature(CELSIUS_OFFSET + temperatureCelsius);
      }
    } catch (NullPointerException e) {
      /* probably caused by a malformed value - ignore */
    } finally {
      safeDestruct.decrement();
    }
  }

  /**
   * Close the failed connection and ask for a new one.  Android needs
   * the old client interface released before it will hand out
   * another, so the close is not optional.
   */
  private void retryConnect() {
    new Handler(Looper.getMainLooper()).post(new Runnable() {
      @Override
      public void run() {
        if (!safeDestruct.increment())
          /* close() got there first */
          return;

        try {
          if (gatt != null) {
            retired = gatt;
            gatt.close();
            gatt = null;
          }

          try {
            connectGatt();
          } catch (IOException e) {
            submitError(e.getMessage() != null
                        ? e.getMessage()
                        : "Bluetooth GATT connect failed");
            return;
          }
        } finally {
          safeDestruct.decrement();
        }
      }
    });
  }

  @Override
  public void onConnectionStateChange(BluetoothGatt gatt,
                                      int status, int newState) {
    if (shutdown)
      return;

    final BluetoothGatt current = this.gatt;
    if (gatt == retired || (current != null && gatt != current))
      /* a disconnect still in flight from the client retryConnect()
         has already closed.  Acting on it would close its replacement
         and spend another retry on a connection that is fine.  The
         first test catches the gap in retryConnect() where the old
         client is closed and #gatt is not yet reassigned; without it
         a stale callback in that gap would pass as the constructor's
         first one, which is the only case the null #gatt means. */
      return;

    if (BluetoothProfile.STATE_CONNECTED == newState &&
        BluetoothGatt.GATT_SUCCESS == status) {
      everConnected = true;
      connectRetries = 0;

      if (Build.VERSION.SDK_INT >= 21)
        gatt.requestConnectionPriority(BluetoothGatt.CONNECTION_PRIORITY_HIGH);

      if (!gatt.discoverServices())
        submitError("Discovering GATT services request failed");

      return;
    }

    if (BluetoothProfile.STATE_DISCONNECTED != newState)
      /* CONNECTING or DISCONNECTING: on the way somewhere, and not a
         state worth reporting either way */
      return;

    if (!everConnected && status != BluetoothGatt.GATT_SUCCESS &&
        connectRetries < MAX_CONNECT_RETRIES) {
      ++connectRetries;
      Log.d(TAG, "BLE sensor GATT disconnected status=" + status +
            ", retry " + connectRetries);
      retryConnect();
      return;
    }

    submitError(BluetoothGatt.GATT_SUCCESS == status
                ? "GATT disconnected"
                : "GATT connection failed (status " + status + ")");
  }

  @Override
  public void onDescriptorWrite(BluetoothGatt gatt,
                                BluetoothGattDescriptor descriptor,
                                int status) {
    synchronized(enableNotificationQueue) {
      currentEnableNotification = enableNotificationQueue.poll();
      if (currentEnableNotification != null) {
        if (!doEnableNotification(currentEnableNotification))
          currentEnableNotification = null;
      }
    }
  }

  @Override
  public void onServicesDiscovered(BluetoothGatt gatt,
                                   int status) {
    if (BluetoothGatt.GATT_SUCCESS != status) {
      submitError("Discovering GATT services failed");
      return;
    }

    /** Check if we know the discovered characteristics, if known,
    * enable notification. Consecutive calls to getServices() 
    * might fail, so we do it just once and handle the lookups in 
    * the loops.
    */
    List<BluetoothGattService> services = gatt.getServices();
    for (BluetoothGattService s : services) {
        List<BluetoothGattCharacteristic> characteristics = s.getCharacteristics();
        for (BluetoothGattCharacteristic c : characteristics) {
            UUID id = c.getUuid();
            for (UUID supported_id : BluetoothUuids.getAllCharacteristicsUuids()) {
              if(id.equals(supported_id)){
                setStateSafe(STATE_READY);
                enableNotification(c);                
              }
            }
       }
    }

    if (state == STATE_LIMBO)
      submitError("Unsupported Bluetooth device");
  }
}
