// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "KeyboardState.hpp"
#include "Translate.hpp"
#include "util/ScopeExit.hxx"

#include <fcntl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace UI {

KeyboardState::KeyboardState(const Keymap *source) noexcept
{
  if (source != nullptr) {
    keymap = *source;
    has_keymap = true;
  }
}

void
KeyboardState::Initialise() noexcept
{
  static_assert(KEY_CNT == 0x300);
  Reset();
  has_keymap = false;
#ifndef KOBO
  for (const char *path : {"/dev/tty", "/dev/tty0"}) {
    const int fd = open(path, O_RDONLY | O_NOCTTY | O_CLOEXEC);
    if (fd < 0)
      continue;
    AtScopeExit(fd) { close(fd); };

    kbentry entry{};
    if (ioctl(fd, KDGKBENT, &entry) < 0)
      continue; // a terminal emulator is not a Linux virtual console

    for (unsigned table = 0; table < keymap.size(); ++table)
      for (unsigned key = 0; key < keymap[table].size(); ++key) {
        entry.kb_table = table;
        entry.kb_index = key;
        if (ioctl(fd, KDGKBENT, &entry) < 0)
          return;
        keymap[table][key] = entry.kb_value;
      }
    has_keymap = true;
    return;
  }
#endif
}

void
KeyboardState::Reset() noexcept
{
  pressed.reset();
  caps_lock = false;
  num_lock = false;
}

void
KeyboardState::Update(unsigned key, bool down) noexcept
{
  if (key >= pressed.size() || pressed[key] == down)
    return; // repeat events must not toggle locks or count modifiers again
  pressed[key] = down;
  if (down && key == KEY_CAPSLOCK)
    caps_lock = !caps_lock;
  if (down && key == KEY_NUMLOCK)
    num_lock = !num_lock;
}

unsigned
KeyboardState::GetCharacter(unsigned key) const noexcept
{
  if (key >= pressed.size() ||
      pressed[KEY_LEFTCTRL] || pressed[KEY_RIGHTCTRL] ||
      pressed[KEY_LEFTMETA] || pressed[KEY_RIGHTMETA])
    return 0;

  const bool shift = pressed[KEY_LEFTSHIFT] || pressed[KEY_RIGHTSHIFT];
  if (has_keymap) {
    if (key >= keymap[0].size())
      return 0;
    unsigned table = unsigned(shift) | (pressed[KEY_RIGHTALT] ? 2U : 0U);
    auto value = keymap[table][key];
    if (caps_lock && KTYP(value) == KT_LETTER)
      value = keymap[table ^ 1][key];
    if (KTYP(value) == KT_PAD) {
      if (value == K_PPLUS)
        return '+';
      if (!num_lock)
        return 0;
      if (value == K_PDOT)
        return '.';
      const unsigned digit = KVAL(value);
      return digit <= KVAL(K_P9) ? '0' + digit : 0;
    }
    // shortcut: ASCII only; add composition when text editors support it.
    return (KTYP(value) == KT_LATIN || KTYP(value) == KT_LETTER) &&
      KVAL(value) >= 0x20 && KVAL(value) <= 0x7e ? KVAL(value) : 0;
  }

  if (pressed[KEY_RIGHTALT])
    return 0; // the US fallback has no AltGr table
  if (key == KEY_KPPLUS)
    return '+';
  if (num_lock) {
    if (key == KEY_KPDOT)
      return '.';
    constexpr unsigned keypad_digits[] = {
      KEY_KP0, KEY_KP1, KEY_KP2, KEY_KP3, KEY_KP4,
      KEY_KP5, KEY_KP6, KEY_KP7, KEY_KP8, KEY_KP9,
    };
    for (unsigned digit = 0; digit < std::size(keypad_digits); ++digit)
      if (key == keypad_digits[digit])
        return '0' + digit;
  }

  const auto [character, is_char] = TranslateKeyCode(key, shift);
  if (!is_char)
    return 0;
  if (character >= 'A' && character <= 'Z' && shift == caps_lock)
    return character + ('a' - 'A');
  return character;
}

} // namespace UI
