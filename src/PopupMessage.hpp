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

#ifndef XCSOAR_POPUP_MESSAGE_H
#define XCSOAR_POPUP_MESSAGE_H

#include "Screen/PaintWindow.hpp"
#include "Renderer/TextRenderer.hpp"
#include "Thread/Mutex.hpp"
#include "Util/StaticString.hxx"

#include <tchar.h>

struct UISettings;
struct DialogLook;
class SingleWindow;

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
class PopupMessage : public PaintWindow
{
public:
  enum Type {
    MSG_UNKNOWN,
    MSG_USERINTERFACE,
  };

private:
  static constexpr unsigned MAXMESSAGES = 20;

  struct Message {
    Type type;
    unsigned tstart; // time message was created
    unsigned texpiry; // time message will expire
    unsigned tshow; // time message is visible for

    StaticString<256u> text;

    Message()
      :type(MSG_UNKNOWN), tstart(0), texpiry(0)
    {
      text.clear();
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
    bool IsNewlyExpired(unsigned now) const {
      return texpiry <= now && texpiry > tstart;
    }

    void Set(Type type, unsigned tshow, const TCHAR *text, unsigned now);

    /**
     * @return true if something was changed
     */
    bool Update(unsigned now);

    /**
     * @return true if a message has been appended
     */
    bool AppendTo(StaticString<2000> &buffer, unsigned now);
  };

  SingleWindow &parent;
  const DialogLook &look;

  PixelRect rc; // maximum message size

  TextRenderer renderer;

  const UISettings &settings;

  Mutex mutex;
  struct Message messages[MAXMESSAGES];
  StaticString<2000> text;

  unsigned n_visible;

  bool enable_sound;

public:
  PopupMessage(SingleWindow &_parent, const DialogLook &_look,
               const UISettings &settings);

  void Create(const PixelRect _rc);

  void UpdateLayout(PixelRect _rc);

  /** returns true if messages have changed */
  bool Render();

protected:
  /** Caller must hold the lock. */
  void AddMessage(unsigned tshow, Type type, const TCHAR *Text);

public:
  void AddMessage(const TCHAR* text, const TCHAR *data=nullptr);

  /**
   * Repeats last non-visible message of specified type
   * (or any message type=MSG_UNKNOWN).
   */
  void Repeat(Type type=MSG_UNKNOWN);

  /** Clears all visible messages (of specified type or if type=0, all). */
  bool Acknowledge(Type type=MSG_UNKNOWN);

private:
  gcc_pure
  unsigned CalculateWidth() const;

  gcc_pure
  PixelRect GetRect(unsigned width, unsigned height) const;

  gcc_pure
  PixelRect GetRect() const;

  void UpdateTextAndLayout();
  int GetEmptySlot();

protected:
  /* virtual methods from class Window */
  bool OnMouseDown(PixelPoint p) override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) override;
};

#endif
