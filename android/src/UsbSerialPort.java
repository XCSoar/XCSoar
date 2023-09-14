// SPDX-License-Identifier: GPL-2.0-only
// Copyright The XCSoar Project

package org.xcsoar;

import android.annotation.TargetApi;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbManager;
import android.os.Build;

import com.felhr.usbserial.UsbSerialDevice;
import com.felhr.usbserial.UsbSerialInterface;

import java.util.Arrays;


@TargetApi(Build.VERSION_CODES.HONEYCOMB_MR1)
public final class UsbSerialPort
  implements AndroidPort, UsbSerialInterface.UsbReadCallback
{
  private final UsbSerialHelper.UsbDeviceInterface parent;
  private UsbDeviceConnection connection;
  private UsbSerialDevice serialDevice;
  private PortListener portListener;
  private InputListener inputListener;
  private int baudRate;
  private int state = STATE_LIMBO;
  private final SafeDestruct safeDestruct = new SafeDestruct();

  public UsbSerialPort(UsbSerialHelper.UsbDeviceInterface parent, int baud) {
    this.parent = parent;
    this.baudRate = baud;
  }

  public synchronized boolean isOpen() {
    return serialDevice != null;
  }

  public synchronized void open(UsbManager manager, UsbDevice device, int iface) {
    connection = manager.openDevice(device);
    if (connection == null) {
      setState(STATE_FAILED);
      return;
    }

    serialDevice = UsbSerialDevice.createUsbSerialDevice(device, connection, iface);
    if (serialDevice == null) {
      setState(STATE_FAILED);
      return;
    }

    if (!serialDevice.open()) {
      serialDevice = null;
      setState(STATE_FAILED);
      return;
    }

    serialDevice.setBaudRate(getBaudRate());
    serialDevice.setDataBits(UsbSerialInterface.DATA_BITS_8);
    serialDevice.setStopBits(UsbSerialInterface.STOP_BITS_1);
    serialDevice.setParity(UsbSerialInterface.PARITY_NONE);
    serialDevice.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF);
    serialDevice.read(this);

    setState(STATE_READY);
  }

  public synchronized void close() {
    parent.portClosed(this);

    safeDestruct.beginShutdown();

    if( serialDevice != null) {
      serialDevice.close();
      serialDevice = null;
    }

    if (connection != null) {
      connection.close();
      connection = null;
    }

    safeDestruct.finishShutdown();
  }

  /**
   * Called by UsbSerialHelper after the device got disconnected.
   */
  public void onDisconnect() {
    error("Disconnected");
  }

  @Override
  public void setListener(PortListener listener) {
    portListener = listener;
  }

  @Override
  public void setInputListener(InputListener listener) {
    inputListener = listener;
  }

  @Override
  public int getState() {
    return state;
  }

  @Override
  public boolean drain() {
    return false;
  }

  @Override
  public int getBaudRate() {
    return baudRate;
  }

  @Override
  public boolean setBaudRate(int baud) {
    serialDevice.setBaudRate(baud);
    baudRate = baud;
    return true;
  }

  @Override
  public synchronized int write(byte[] data, int length) {
    serialDevice.write(Arrays.copyOf(data, length));
    return length;
  }

  @Override
  public void onReceivedData(byte[] arg0) {
    if (arg0.length == 0) {
      error("Disconnected");
      return;
    }

    InputListener listener = inputListener;
    if (listener != null && safeDestruct.increment()) {
      try {
        listener.dataReceived(arg0, arg0.length);
      } finally {
        safeDestruct.decrement();
      }
    }
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

  protected void setState(int newState) {
    if (newState == state)
      return;

    state = newState;
    stateChanged();
  }

  protected void error(String msg) {
    state = STATE_FAILED;

    PortListener portListener = this.portListener;
    if (portListener != null && safeDestruct.increment()) {
      try {
        portListener.portError(msg);
      } finally {
        safeDestruct.decrement();
      }
    }
  }
}
