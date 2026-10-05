// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

/**
 * A description for a font that shall be loaded.
 */
class FontDescription {
  unsigned height;
  bool bold, italic;
  bool monospace;
  float letter_spacing_em;

public:
  FontDescription() = default;

  /**
   * @param _height the "em" height of the font
   */
  explicit constexpr FontDescription(unsigned _height,
                                     bool _bold=false, bool _italic=false,
                                     bool _monospace=false,
                                     float _letter_spacing_em=0.f)
    :height(_height), bold(_bold), italic(_italic), monospace(_monospace),
     letter_spacing_em(_letter_spacing_em) {}

  constexpr unsigned GetHeight() const {
    return height;
  }

  void SetHeight(unsigned _height) {
    height = _height;
  }

  constexpr FontDescription WithHeight(unsigned _height) const {
    return FontDescription(_height, bold, italic, monospace,
                           letter_spacing_em);
  }

  constexpr bool IsBold() const {
    return bold;
  }

  void SetBold(bool _bold=true) {
    bold = _bold;
  }

  constexpr FontDescription WithBold(bool _bold=true) const {
    return FontDescription(height, _bold, italic, monospace,
                           letter_spacing_em);
  }

  constexpr float GetLetterSpacing() const {
    return letter_spacing_em;
  }

  void SetLetterSpacing(float em) {
    letter_spacing_em = em;
  }

  constexpr bool IsItalic() const {
    return italic;
  }

  constexpr bool IsMonospace() const {
    return monospace;
  }
};
