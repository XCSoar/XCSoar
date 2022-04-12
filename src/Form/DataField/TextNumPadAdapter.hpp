/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef SRC_FORM_DATAFIELD_TEXTNUMPADADAPTER_HPP_
#define SRC_FORM_DATAFIELD_TEXTNUMPADADAPTER_HPP_

#include "Form/DataField/Base.hpp"
#include "ui/event/Timer.hpp"

static const unsigned MAX_CHARS_PER_KEY = 5;
static const unsigned MAX_BUTTONS = 10;
static const unsigned NO_PREVIOUSBUTTON = 1000;
static constexpr size_t MAX_TEXTENTRY = 100;
static const std::chrono::steady_clock::duration TIMEOUT_FOR_KEYPRESSED= std::chrono::seconds(1);

class TextNumPadAdapter : public NumPadAdapter{
  typedef bool (*OnCharacterCallback_t)(unsigned ch);

  typedef std::function<const TCHAR *(const TCHAR *)> AllowedCharacters;
protected:
  AllowedCharacters NumPadAllowedCharactersCallback;
  OnCharacterCallback_t on_character;
  bool shift_state;
  const bool show_shift_button;
  void CheckKey(TCHAR *output, const TCHAR *allowedCharacters,
                                const TCHAR key) const noexcept;
	void UpdateAllowedCharacters( const TCHAR  *dataFieldContent)  noexcept;
	unsigned GetButtonIndex(unsigned row, unsigned column) const noexcept;
	unsigned GetRowFromButtonIndex(unsigned buttonIndex) const noexcept;

	unsigned GetColumnFromButtonIndex(unsigned buttonIndex) const noexcept;
	void OnDataFieldSetFocus() noexcept override;
	public:
	TextNumPadAdapter( NumPadWidgetInterface * _numPadWidgetInterface, AllowedCharacters acb,
                 bool _show_shift_button,
                 bool _default_shift_state = true);
	~TextNumPadAdapter()noexcept{};
	void UpdateButtons() noexcept override;
	bool CharacterFunction(unsigned ch) noexcept override;
	bool
	OnKeyDown(unsigned key_code) noexcept override;
	bool OnKeyCheck(unsigned key_code) const noexcept override;
private:
	UI::Timer keyPressedTimer;
	unsigned selectedButtonIndex;
	bool numPadEditingActive;
	void SelectNextButton() noexcept;
	void SelectPreviousButton() noexcept;
	void OnSelectedButton() noexcept;
	void
	SetCharFromKeyPress(const TCHAR * allowedCharactersForCurrentKey) noexcept;
	void SetCaption(unsigned buttonIndex, const TCHAR *allowedChars) const noexcept;
  unsigned previousButtonIndex;
  unsigned previousKeyIndex;
  void OnKeyBack()noexcept;
  void OnKeyEdit() noexcept;

	void OnNewKey()noexcept; // User pressed a different key
	void OnButton(unsigned ButtonIndex)noexcept override;
  void KeyFinished()noexcept; // User stopped pressing the same key
	void BeginEditing() noexcept override;
	void EndEditing() noexcept  override;
  const TCHAR *GetAllowedCharacters(const TCHAR *prefix)noexcept;
};



#endif /* SRC_FORM_DATAFIELD_TEXTNUMPADADAPTER_HPP_ */
