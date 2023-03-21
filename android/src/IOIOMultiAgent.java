// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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
