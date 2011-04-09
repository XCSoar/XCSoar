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

#ifndef XCSOAR_POPUP_MESSAGE_H
#define XCSOAR_POPUP_MESSAGE_H

#include "Thread/Mutex.hpp"
#include "PeriodClock.hpp"
#include "Screen/EditWindow.hpp"

#include <tchar.h>

class SingleWindow;
class StatusMessageList;

/**
 * - Single window, created in GUI thread.
 *    -- hidden when no messages for display
 *    -- shown when messages available
 *    -- disappear when touched
 *    -- disappear when return clicked
 *    -- disappear when timeout
 *    -- disappear when extrn event triggered
 * - Message properties
 *    -- have a start time (seconds)
 *    -- timeout (start time + delta)
 * - Messages stay in a circular buffer can be reviewed
 * - Optional logging of all messages to file
 * - Thread locking so available from any thread
 */
class PopupMessage : public EditWindow {
public:
  enum {
    MSG_UNKNOWN=0,
    MSG_AIRSPACE=1,
    MSG_USERINTERFACE=2,
    MSG_GLIDECOMPUTER=3,
    MSG_COMMS=4
  };

private:
  enum { MAXMESSAGES = 20 };

  struct Message {
    TCHAR text[1000];
    int type;
    int tstart; // time message was created
    int texpiry; // time message will expire
    int tshow; // time message is visible for

    Message()
      :type(MSG_UNKNOWN), tstart(0), texpiry(0) {
      text[0] = _T('\0');
    }

    bool IsUnknown() const {
      return type == MSG_UNKNOWN;
    }

    bool IsNew() const {
      return texpiry == tstart;
    }

    /**
     * Expired for the first time?
     */
    bool IsNewlyExpired(int now) const {
      return texpiry <= now && texpiry > tstart;
    }

    void Set(int type, int tshow, const TCHAR *text, int now);

    /**
     * @return true if something was changed
     */
    bool Update(int now);

    /**
     * @return true if a message has been appended
     */
    bool AppendTo(TCHAR *buffer, int now);
  };

  PeriodClock clock;

  const StatusMessageList &status_messages;

  SingleWindow &parent;
  PixelRect rc; // maximum message size

  Mutex mutex;
  struct Message messages[MAXMESSAGES];
  TCHAR msgText[2000];

  unsigned nvisible;

  bool enable_sound;

public:
  PopupMessage(const StatusMessageList &_status_messages,
               SingleWindow &_parent);

  void set(const PixelRect _rc);

  /** returns true if messages have changed */
  bool Render();

  void AddMessage(int tshow, int type, const TCHAR *Text);
  void AddMessage(const TCHAR* text, const TCHAR *data=NULL);

  /**
   * Repeats last non-visible message of specified type (or any
   * message type=0).
   */
  void Repeat(int type);

  /**
   * Clears all visible messages (of specified type or if type=0,
   * all).
   */
  bool Acknowledge(int type);

 private:
  void Resize();
  int GetEmptySlot();

protected:
  virtual bool on_mouse_down(int x, int y);
};

#endif
