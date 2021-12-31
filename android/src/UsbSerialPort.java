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
  public UsbSerialPort(UsbDevice device,int baud) {
    _UsbDevice = device;
    _baudRate = baud;
  }

  private UsbDevice _UsbDevice;
  private UsbDeviceConnection _UsbConnection;
  private UsbSerialDevice _SerialPort;
  private PortListener portListener;
  private InputListener inputListener;
  private int _baudRate;
  private int state = STATE_LIMBO;
  private final SafeDestruct safeDestruct = new SafeDestruct();

  public synchronized void open(UsbManager manager) {
    _UsbConnection = manager.openDevice(_UsbDevice);
    if (_UsbConnection == null) {
      setState(STATE_FAILED);
      return;
    }

    _SerialPort = UsbSerialDevice.createUsbSerialDevice(_UsbDevice, _UsbConnection);
    if (_SerialPort == null) {
      setState(STATE_FAILED);
      return;
    }

    if (!_SerialPort.open()) {
      setState(STATE_FAILED);
      return;
    }

    _SerialPort.setBaudRate(getBaudRate());
    _SerialPort.setDataBits(UsbSerialInterface.DATA_BITS_8);
    _SerialPort.setStopBits(UsbSerialInterface.STOP_BITS_1);
    _SerialPort.setParity(UsbSerialInterface.PARITY_NONE);
    _SerialPort.setFlowControl(UsbSerialInterface.FLOW_CONTROL_OFF);
    _SerialPort.read(this);

    setState(STATE_READY);
  }

  public synchronized void close() {
    safeDestruct.beginShutdown();

    if( _SerialPort != null) {
      _SerialPort.close();
      _SerialPort = null;
    }

    if (_UsbConnection != null) {
      _UsbConnection.close();
      _UsbConnection = null;
    }

    safeDestruct.finishShutdown();
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
    return _baudRate;
  }

  @Override
  public boolean setBaudRate(int baud) {
    _SerialPort.setBaudRate(baud);
    _baudRate =  baud;
    return true;
  }

  @Override
  public synchronized int write(byte[] data, int length) {
    _SerialPort.write(Arrays.copyOf(data, length));
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
