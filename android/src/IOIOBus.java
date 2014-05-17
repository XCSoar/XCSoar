/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

import ioio.lib.api.IOIO;
import ioio.lib.api.DigitalInput;
import ioio.lib.api.TwiMaster;
import ioio.lib.api.SpiMaster;
import ioio.lib.api.exception.ConnectionLostException;

/**
 * Common interface for IIC and SPI buses connected through IOIO.
 *
 * We use a slow 100 KHz clock because of radio interference reasons.
 */
abstract class IOIOBus
{
  protected int addr;
  private DigitalInput eoc;

  protected byte[] dummy = new byte [0];
  
  static public IOIOBus Create(IOIO ioio, int bus, int addr)
	throws ConnectionLostException {
	  if (bus >= 0x20) return new IOIOBusSPI(ioio, bus - 0x20, addr);
	  else return new IOIOBusI2C(ioio, bus, addr);
  }
  protected IOIOBus(IOIO ioio, int _addr, int _eoc)
    throws ConnectionLostException {
	addr = _addr;
	if (_eoc != 0)
	  eoc = ioio.openDigitalInput(_eoc);
  }
  public void close() {
    if (eoc != null)
    	eoc.close();
  }

  public void write(byte[] data, int size)
    throws ConnectionLostException, InterruptedException {
	  writeRead(data, size, dummy, 0);
  }
  public void read(byte[] data, int size)
    throws ConnectionLostException, InterruptedException {
	  writeRead(dummy, 0, data, size);
  }
  abstract public void writeRead(byte[] wData, int wSize, byte[] rData, int rSize)
    throws ConnectionLostException, InterruptedException;
  public void waitForValue(int time)
    throws ConnectionLostException, InterruptedException {
    if (eoc != null)
      eoc.waitForValue(true);
    else
      Thread.sleep(time);
  }
}

/**
 * IOIO I2C bus interface implementation
 */
final class IOIOBusI2C extends IOIOBus {
  private TwiMaster twi;
  
  public IOIOBusI2C(IOIO ioio, int _bus, int _addr)
	throws ConnectionLostException {
    super(ioio, _addr, (byte)(_bus >> 16));
	
	twi = ioio.openTwiMaster(_bus & 0xff, TwiMaster.Rate.RATE_100KHz, false);
	}
  @Override public void close() {
    if (twi != null)
        twi.close();
    super.close();
  }

  @Override public void writeRead(byte[] wData, int wSize, byte[] rData, int rSize)
    throws ConnectionLostException, InterruptedException {
	if (wSize > 0) twi.writeRead(addr, false, wData, wSize, dummy, 0);
	if (rSize > 0) twi.writeRead(addr, false, dummy, 0, rData, rSize);
//    twi.writeRead(addr, false, wData, wSize, rData, rSize);
  }
}

/**
 * IOIO SPI bus interface implementation
 */
final class IOIOBusSPI extends IOIOBus {
  private SpiMaster spi;

  public IOIOBusSPI(IOIO ioio, int _bus, int _addr)
	throws ConnectionLostException {
    super(ioio, _addr, (byte)(_bus >> 16));
	
    SpiMaster.Rate rate = SpiMaster.Rate.RATE_125K;//SpiMaster.Rate.RATE_1M;
	switch(_bus & 0xff) {
    case 0: spi = ioio.openSpiMaster(3/*miso*/, 4/*mosi*/, 5/*clk*/, 6/*, 7*//*ss*/, rate);
      break;
    case 1: spi = ioio.openSpiMaster(46/*miso*/, 47/*mosi*/, 48/*clk*/, 45/*ss*/, rate);
      break;
    case 2: spi = ioio.openSpiMaster(9/*miso*/, 10/*mosi*/, 11/*clk*/, 12/*, 13, 14*//*ss*/, rate);
      break;
    }
  }
  @Override public void close() {
    if (spi != null)
        spi.close();
    super.close();
  }

  @Override public void writeRead(byte[] wData, int wSize, byte[] rData, int rSize)
    throws ConnectionLostException, InterruptedException {
    spi.writeRead(addr, wData, wSize, wSize + rSize, rData, rSize);
  }
}
