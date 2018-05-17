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

#ifndef XCSOAR_DEVICE_ANDROID_IOIO_UART_PORT_HPP
#define XCSOAR_DEVICE_ANDROID_IOIO_UART_PORT_HPP

#include "Compiler.h"

#include <tchar.h>

class Port;
class PortListener;
class DataHandler;

namespace AndroidIOIOUartPort
{
  static inline unsigned getNumberUarts() { return 4; }

  static inline const TCHAR *getPortHelp(unsigned UartID) {
    switch (UartID) {
    case 0:
      return _T("IOIO external board Uart: pin3=out, pin4=in");
    case 1:
      return _T("IOIO external board Uart: pin5=out, pin6=in");
    case 2:
      return _T("IOIO external board Uart: pin10=out, pin11=in");
    case 3:
      return _T("IOIO external board Uart: pin12=out, pin13=in");
    default:
      return _T("Illegal IOIO Uart ID");
    }
  }
}

gcc_malloc
Port *
OpenAndroidIOIOUartPort(unsigned uart_id, unsigned baud_rate,
                        PortListener *listener, DataHandler &handler);

#endif
