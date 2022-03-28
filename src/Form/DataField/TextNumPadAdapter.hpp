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

class TextNumPadAdapter : public NumPadAdapter{
  typedef bool (*OnCharacterCallback_t)(unsigned ch);
  typedef std::function<const TCHAR *(const TCHAR *)> AllowedCharacters;

protected:
  static constexpr unsigned MAX_BUTTONS = 10;
  AllowedCharacters NumPadAllowedCharactersCallback;
  OnCharacterCallback_t on_character;
  bool shift_state;
  const bool show_shift_button;
  void CheckKey(TCHAR *output, const TCHAR *allowedCharacters,
                                const TCHAR key) const noexcept;
	TCHAR UpdateAllowedCharacters()  noexcept;

	public:
	TextNumPadAdapter( NumPadWidgetInterface * _numPadWidgetInterface, AllowedCharacters acb,
                 bool _show_shift_button,
                 bool _default_shift_state = true);
	void UpdateButtons() noexcept override;
	bool
	KeyPress(unsigned key_code) noexcept;
private:	
	void
	setCharFromKeyPress(unsigned key_code,
                                           const TCHAR *keys) noexcept;

};



#endif /* SRC_FORM_DATAFIELD_TEXTNUMPADADAPTER_HPP_ */
