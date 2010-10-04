/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#ifndef XCSOAR_DEVICE_PORT_HPP
#define XCSOAR_DEVICE_PORT_HPP

#include <stddef.h>

/**
 * Generic Port thread handler class
 */
class Port {
public:
  /**
   * Interface with callbacks for the #Port class.
   */
  class Handler {
  public:
    virtual void LineReceived(const char *line) = 0;
  };

protected:
  Handler &handler;

public:
  Port(Handler &_handler);
  virtual ~Port();

  /**
   * Writes a string to the serial port
   * @param data Pointer to the first character
   * @param length Length of the string
   */
  virtual void Write(const void *data, unsigned length) = 0;

  /**
   * Writes a null-terminated string to the serial port
   * @param s The string to write
   */
  void Write(const char *s);

  /**
   * Writes a single byte to the serial port
   * @param ch Byte to write
   */
  void Write(char ch) {
    Write(&ch, sizeof(ch));
  }

  /**
   * Flushes the serial port buffers
   */
  virtual void Flush() = 0;

  /**
   * Sets the RX timeout in ms
   * @param Timeout The receive timeout in ms
   * @return The previous timeout in ms or -1 on error
   */
  virtual int SetRxTimeout(int Timeout) = 0;

  /**
   * Sets the baud rate of the serial port to the given value
   * @param BaudRate The desired baudrate
   * @return The previous baud rate or 0 on error
   */
  virtual unsigned long SetBaudrate(unsigned long BaudRate) = 0;

  /**
   * Stops the receive thread
   * @return True on success, False on failure
   */
  virtual bool StopRxThread() = 0;

  /**
   * (Re)Starts the receive thread
   * @return True on success, False on failure
   */
  virtual bool StartRxThread() = 0;

  /**
   * Read a single byte from the serial port
   * @return The byte that was read or EOF in failure
   */
  virtual int GetChar() = 0;

  /**
   * Read data from the serial port
   * @param Buffer Pointer to the buffer
   * @param Size Size of the buffer
   * @return Number of bytes read from the serial port
   */
  virtual int Read(void *Buffer, size_t Size) = 0;

  bool ExpectString(const char *token);
};

#endif
