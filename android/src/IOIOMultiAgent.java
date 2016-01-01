/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

import java.util.ArrayList;
import java.util.Collection;
import ioio.lib.api.IOIO;
import ioio.lib.api.exception.ConnectionLostException;
import ioio.lib.spi.IOIOConnectionFactory;

/**
 * This class is a container for multiple IOIOAgent objects.
 */
final class IOIOMultiAgent implements IOIOAgent.Listener {
  private final ArrayList<IOIOAgent> agents;

  private final IOIOAgent.Listener listener;

  private IOIO ioio_;

  public IOIOMultiAgent(Collection<IOIOConnectionFactory> factories,
                        IOIOAgent.Listener _listener) {
    agents = new ArrayList<IOIOAgent>(factories.size());
    listener = _listener;

    for (IOIOConnectionFactory factory : factories)
      agents.add(new IOIOAgent(factory, this));
  }

  public void enable() {
    for (IOIOAgent agent : agents)
      agent.enable();
  }

  public void disable() {
    for (IOIOAgent agent : agents)
      agent.disable();
  }

  public void shutdown() {
    for (IOIOAgent agent : agents)
      agent.shutdown();
  }

  public void wakeUp() {
    for (IOIOAgent agent : agents)
      agent.wakeUp();
  }

  @Override public synchronized void onIOIOConnect(IOIO ioio)
    throws ConnectionLostException, InterruptedException {
    if (ioio_ != null)
      return;

    ioio_ = ioio;
    listener.onIOIOConnect(ioio);
  }

  @Override public synchronized void onIOIODisconnect(IOIO ioio) {
    if (ioio != ioio_)
      return;

    ioio_ = null;
    listener.onIOIODisconnect(ioio);
  }

  @Override public synchronized boolean onIOIOIdle(IOIO ioio)
    throws ConnectionLostException, InterruptedException {
    if (ioio != ioio_)
      return false;

    return listener.onIOIOIdle(ioio);
  }
}
