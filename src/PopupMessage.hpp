// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/window/PaintWindow.hpp"
#include "Operation/Operation.hpp"
#include "thread/Mutex.hxx"
#include "util/StaticArray.hxx"
#include "util/StaticString.hxx"

#include <chrono>
#include <memory>

struct UISettings;
struct DialogLook;
class Font;
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
  static constexpr unsigned MAX_PILLS = 8;

  struct Message {
    Type type = MSG_UNKNOWN;
    std::chrono::steady_clock::time_point tstart{}; // time message was created
    std::chrono::steady_clock::time_point texpiry{}; // time message will expire
    std::chrono::steady_clock::duration tshow; // time message is visible for

    StaticString<256u> text;

    /** From #StatusMessage::sound; nullptr if none or not from status file. */
    const char *sound = nullptr;

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
             std::chrono::steady_clock::time_point now,
             const char *_sound = nullptr) noexcept;

    /**
     * @return true if something was changed
     */
    bool Update(std::chrono::steady_clock::time_point now) noexcept;

    [[gnu::pure]]
    bool IsVisible(std::chrono::steady_clock::time_point now) const noexcept;
  };

  UI::SingleWindow &parent;
  const DialogLook &look;

  PixelRect rc; // HUD rectangle (parent coordinates)

  const UISettings &settings;

  Mutex mutex;
  struct Message messages[MAXMESSAGES];

  StaticArray<StaticString<256>, MAX_PILLS> status_pills;

  class SlotEnv final : public QuietOperationEnvironment {
    PopupMessage &host;
    const void *key;

  public:
    SlotEnv(PopupMessage &_host, const void *_key) noexcept
      :host(_host), key(_key) {}

    void SetText(const char *text) noexcept override {
      host.SetProgressText(key, text);
    }

    void SetProgressRange(unsigned range) noexcept override {
      host.SetProgressRange(key, range);
    }

    void SetProgressPosition(unsigned position) noexcept override {
      host.SetProgressPosition(key, position);
    }
  };

  struct ProgressSlot {
    const void *key = nullptr;
    StaticString<128> text;
    unsigned range = 0;
    unsigned position = 0;
    /** {} while live; set by ClearProgress to linger at 100% */
    std::chrono::steady_clock::time_point hold_until{};
    std::unique_ptr<SlotEnv> env;
  };

  StaticArray<ProgressSlot, MAX_PILLS> progress_slots;

  /** when was the last haptic feedback for a new message generated? */
  std::chrono::steady_clock::time_point last_haptic{};

  bool enable_sound = true;

public:
  PopupMessage(UI::SingleWindow &_parent, const DialogLook &_look,
               const UISettings &settings) noexcept;

  void Create(const PixelRect _rc) noexcept;

  void UpdateLayout(PixelRect _rc) noexcept;

  /** returns true if messages have changed */
  bool Render() noexcept;

  void BeginProgress(const void *key, const char *text) noexcept;
  void ClearProgress(const void *key) noexcept;
  OperationEnvironment &GetProgressEnvironment(const void *key) noexcept;
  void SetProgressText(const void *key, const char *text) noexcept;
  void SetProgressRange(const void *key, unsigned range) noexcept;
  void SetProgressPosition(const void *key, unsigned position) noexcept;

protected:
  /** Caller must hold the lock. */
  void AddMessage(std::chrono::steady_clock::duration tshow, Type type,
                  const char *Text,
                  const char *sound = nullptr) noexcept;

public:
  void AddMessage(const char* text, const char *data=nullptr) noexcept;

  /**
   * Repeats last non-visible message of specified type
   * (or any message type=MSG_UNKNOWN), including its status sound if any.
   */
  void Repeat(Type type=MSG_UNKNOWN) noexcept;

  /** Clears all visible messages (of specified type or if type=0, all). */
  bool Acknowledge(Type type=MSG_UNKNOWN) noexcept;

private:
  [[gnu::pure]]
  int FindProgressLocked(const void *key) const noexcept;

  [[gnu::pure]]
  bool HasPills() const noexcept;
  [[gnu::pure]]
  const Font &GetFont() const noexcept;

  [[gnu::pure]]
  PixelRect GetRect() const noexcept;

  void CollectStatusPills() noexcept;
  void UpdateTextAndLayout() noexcept;
  int GetEmptySlot() noexcept;

protected:
  /* virtual methods from class Window */
  bool OnMouseDown(PixelPoint p) noexcept override;

  /* virtual methods from class PaintWindow */
  void OnPaint(Canvas &canvas) noexcept override;
};
