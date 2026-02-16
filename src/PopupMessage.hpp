// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "Renderer/TextRenderer.hpp"
#include "thread/Mutex.hxx"
#include "util/StaticString.hxx"

#include <chrono>

#include <tchar.h>

struct UISettings;
struct DialogLook;
namespace UI { class SingleWindow; }

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
    Type type = MSG_UNKNOWN;
    std::chrono::steady_clock::time_point tstart{}; // time message was created
    std::chrono::steady_clock::time_point texpiry{}; // time message will expire
    std::chrono::steady_clock::duration tshow; // time message is visible for

    StaticString<256u> text;

    constexpr Message() noexcept
    {
      text.clear();
    }

    constexpr bool IsUnknown() const noexcept {
      return type == MSG_UNKNOWN;
    }

    constexpr bool IsNew() const noexcept {
      return texpiry == tstart;
    }

    /**
     * Expired for the first time?
     */
    constexpr bool IsNewlyExpired(std::chrono::steady_clock::time_point now) const noexcept {
      return texpiry <= now && texpiry > tstart;
    }

    void Set(Type type, std::chrono::steady_clock::duration tshow,
             const char *text,
             std::chrono::steady_clock::time_point now) noexcept;

    /**
     * @return true if something was changed
     */
    bool Update(std::chrono::steady_clock::time_point now) noexcept;

    /**
     * @return true if a message has been appended
     */
    bool AppendTo(StaticString<2000> &buffer,
                  std::chrono::steady_clock::time_point now) noexcept;
  };

  UI::SingleWindow &parent;
  const DialogLook &look;

  PixelRect rc; // maximum message size

  TextRenderer renderer;

  const UISettings &settings;

  Mutex mutex;
  struct Message messages[MAXMESSAGES];
  StaticString<2000> text;

  unsigned n_visible = 0;

  bool enable_sound = true;

public:
  PopupMessage(UI::SingleWindow &_parent, const DialogLook &_look,
               const UISettings &settings) noexcept;

  void Create(const PixelRect _rc) noexcept;

  void UpdateLayout(PixelRect _rc) noexcept;

  /** returns true if messages have changed */
  bool Render() noexcept;

protected:
  /** Caller must hold the lock. */
  void AddMessage(std::chrono::steady_clock::duration tshow, Type type,
                  const char *Text) noexcept;

public:
  void AddMessage(const char* text, const char *data=nullptr) noexcept;

  /**
   * Repeats last non-visible message of specified type
   * (or any message type=MSG_UNKNOWN).
   */
  void Repeat(Type type=MSG_UNKNOWN) noexcept;

  /** Clears all visible messages (of specified type or if type=0, all). */
  bool Acknowledge(Type type=MSG_UNKNOWN) noexcept;

private:
  [[gnu::pure]]
  unsigned CalculateWidth() const noexcept;

  [[gnu::pure]]
  PixelRect GetRect(PixelSize size) const noexcept;

  [[gnu::pure]]
  PixelRect GetRect() const noexcept;

  void UpdateTextAndLayout() noexcept;
  int GetEmptySlot() noexcept;

protected:
  /* virtual methods from class Window */
  bool OnMouseDown(PixelPoint p) noexcept override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};
