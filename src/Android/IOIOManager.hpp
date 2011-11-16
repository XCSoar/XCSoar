/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef XCSOAR_ANDROID_IOIOMANAGER_HPP
#define XCSOAR_ANDROID_IOIOMANAGER_HPP

#include "Java/Object.hpp"
#include "Java/Ref.hpp"
#include "Android/IOIOHelper.hpp"
#include "Thread/Mutex.hpp"

/**
* Manages the connecting and disconnecting of the IOIO board based
* on the number of client references using the IOIOHelper object.
* Connects to the IOIO board automatically when the first client calls
* AddClient().  Disconnects from the IOIO board when the last client
* calls RemoveClient().
*/
class IOIOManager {
private:
  unsigned ref_count;
  Mutex ref_count_mutex;
  IOIOHelper* helper;

public:
  /**
   * Creates the instance of the IOIOHelper class.  This class will
   * exist for the lifetime of the application.
   * @param env
   * @return
   */
  IOIOManager(JNIEnv *env);

  /**
   * Deletes instance of IOIOHelper object
   * Assumes all open Uarts have been closed
   * before calling
   */
  ~IOIOManager() {
    assert(ref_count == 0);
    delete helper;
  }

  /**
   * Increases reference count of clients of ioio_helper
   * Connects the ioio_helper to the ioio board if first client
   * Returns a pointer to the ioio helper.
   * Must be called immediately before client calls openUart().
   * However, it is possible with multiple threads that openUart()
   * is executed prior to connect() in which case openUart()
   * will fail (and return false), but if the port is closed and reopened
   * by the client, openUart() will work at the second attempt.
   */
  IOIOHelper* AddClient();

  /**
   * Decreases reference count of clients of ioio_helper
   * Disconnects the ioio_helper if last client
   * Must be called immediately after client calls closeUart();
   * However, it is possible with multiple threads that closeUart()
   * is executed after close() in which case closeUart()
   * will generate an exception that is trapped by the Java.
   */
  void RemoveClient();


};

#endif
