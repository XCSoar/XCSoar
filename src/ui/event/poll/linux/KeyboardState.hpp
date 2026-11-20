// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>
#include <bitset>

namespace UI {

/** Modifier and text state for one Linux keyboard. */
class KeyboardState {
public:
  /** Linux console tables: plain, Shift, AltGr, Shift+AltGr. */
  using Keymap = std::array<std::array<unsigned short, 256>, 4>;

private:
  /* Linux KEY_CNT; avoid exporting Linux's key macros through this header. */
  std::bitset<0x300> pressed;
  bool caps_lock = false;
  bool num_lock = false;

  Keymap keymap{};
  bool has_keymap = false;

public:
  KeyboardState() noexcept = default;
  explicit KeyboardState(const Keymap *source) noexcept;
  /** Load configuration before input dispatch, never on a key event. */
  void Initialise() noexcept;
  KeyboardState(const KeyboardState &) noexcept = default;
  KeyboardState &operator=(const KeyboardState &) = delete;

  const Keymap *GetKeymap() const noexcept {
    return has_keymap ? &keymap : nullptr;
  }

  void Reset() noexcept;
  void CopyLocksFrom(const KeyboardState &source) noexcept {
    caps_lock = source.caps_lock;
    num_lock = source.num_lock;
  }
  void Update(unsigned key, bool down) noexcept;
  unsigned GetCharacter(unsigned key) const noexcept;
};

} // namespace UI
