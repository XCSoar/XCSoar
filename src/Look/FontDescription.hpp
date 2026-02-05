// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef USE_GDI
#include <windef.h>
#include <wingdi.h>
#include <tchar.h>

class FontDescription {
  LOGFONT logfont;

public:
  FontDescription() = default;

  /**
   * @param _height the character (="em") height of the font
   */
  explicit FontDescription(unsigned _height,
                           bool _bold=false, bool _italic=false,
                           bool _monospace=false);

  explicit operator const LOGFONT &() const {
    return logfont;
  }

  unsigned GetHeight() const {
    return -logfont.lfHeight;
  }

  void SetHeight(unsigned _height) {
    logfont.lfHeight = -int(_height);
  }

  FontDescription WithHeight(unsigned _height) const {
    FontDescription result(*this);
    result.SetHeight(_height);
    return result;
  }

  void SetBold(bool bold=true) {
    logfont.lfWeight = bold ? FW_BOLD : FW_MEDIUM;
  }

  FontDescription WithBold(bool bold=true) const {
    FontDescription result(*this);
    result.SetBold(bold);
    return result;
  }

private:
  void Init(const char *face,
            int height,
            bool bold, bool italic,
            bool monospace);
};

#else

/**
 * A description for a font that shall be loaded.
 */
class FontDescription {
  unsigned height;
  bool bold, italic;
  bool monospace;

public:
  FontDescription() = default;

  /**
   * @param _height the "em" height of the font
   */
  explicit constexpr FontDescription(unsigned _height,
                                     bool _bold=false, bool _italic=false,
                                     bool _monospace=false)
    :height(_height), bold(_bold), italic(_italic), monospace(_monospace) {}

  constexpr unsigned GetHeight() const {
    return height;
  }

  void SetHeight(unsigned _height) {
    height = _height;
  }

  constexpr FontDescription WithHeight(unsigned _height) const {
    return FontDescription(_height, bold, italic, monospace);
  }

  constexpr bool IsBold() const {
    return bold;
  }

  void SetBold(bool _bold=true) {
    bold = _bold;
  }

  constexpr FontDescription WithBold(bool _bold=true) const {
    return FontDescription(height, _bold, italic, monospace);
  }

  constexpr bool IsItalic() const {
    return italic;
  }

  constexpr bool IsMonospace() const {
    return monospace;
  }
};

#endif
