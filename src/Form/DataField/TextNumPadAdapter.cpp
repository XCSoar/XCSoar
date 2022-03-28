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
#include "ui/event/KeyCode.hpp"
#include "Form/DataField/TextNumPadAdapter.hpp"
#include "Renderer/SymbolButtonRenderer.hpp"
#include "util/StringAPI.hxx"
#include "util/StringCompare.hxx"
#include "util/CharUtil.hxx"
#include "Screen/Layout.hpp"
#include "Renderer/TextButtonRenderer.hpp"
#include "ui/window/ContainerWindow.hpp"
#include "Dialogs/DialogSettings.hpp"
#include "UIGlobals.hpp"
#include <time.h>
#include <sys/time.h>
#include <cassert>
#include <string.h>
#include "Form/DataField/NumPadWidgetInterface.hpp"

static constexpr long waitForSameKeyTime = 1000000;// one second = 1000.000 microseconds
static constexpr TCHAR charsForKey[10][5] = { _T("ABC1"), _T("DEF2"), _T(
    "GHI3"),
                                              _T("JKL4"), _T("MNO5"), _T(
                                                  "PQR6"),
                                              _T("STU7"), _T("VWX8"), _T("YZ9"),
                                              _T(" -0") };
TCHAR allowedCharsForKey[10][5];
static constexpr size_t MAX_TEXTENTRY = 40;


TextNumPadAdapter::TextNumPadAdapter( NumPadWidgetInterface * _numPadWidget,
                                     AllowedCharacters acb,
                                     bool _show_shift_button,
                                     bool _default_shift_state
                                     ) :  NumPadAdapter(_numPadWidget), NumPadAllowedCharactersCallback(acb), on_character(
    nullptr), shift_state(_default_shift_state), show_shift_button(
    _show_shift_button)
{
}
;

void
TextNumPadAdapter::CheckKey(TCHAR *output, const TCHAR *allowedCharacters,
                            const TCHAR key) const noexcept
{
  TCHAR cbuf[] = { key, '\0' };
  if (allowedCharacters == nullptr
      || StringFind(allowedCharacters, key, StringLength(allowedCharacters))
          != nullptr)
    UnsafeCopyString(output + StringLength(output), cbuf);
}

TCHAR
TextNumPadAdapter::UpdateAllowedCharacters()  noexcept
{
  const TCHAR *allowed = nullptr;
  if (NumPadAllowedCharactersCallback)
    allowed = NumPadAllowedCharactersCallback(dataField->GetAsString());
  for (unsigned i = 0; i < numPad->GetNumButtons(); ++i) {
    TextButtonRenderer &renderer = (TextButtonRenderer&)numPad->GetButtons()[i].GetRenderer();
    TCHAR caption[7] = _T("0\n");
    allowedCharsForKey[i][0] = '\0';
    for (unsigned c = 0; c < StringLength(charsForKey[i]); c++) {
      CheckKey(allowedCharsForKey[i], allowed, charsForKey[i][c]);
    }
    caption[0] = '0' + ((i + 1) % 10);
    UnsafeCopyString(caption + StringLength(caption), allowedCharsForKey[i]);
    numPad->GetButtons()[i].SetVisible(
        allowed == nullptr || StringLength(caption) == 0);
    numPad->GetButtons()[i].SetVisible(true);
    renderer.SetCaption(caption);
    numPad->GetButtons()->Invalidate();
  }
  if (allowed != nullptr && 1 == StringLength(allowed))
    return allowed[0];
  return 0;
}

void
TextNumPadAdapter::UpdateButtons() noexcept
{
  UpdateAllowedCharacters();
}

void
TextNumPadAdapter::setCharFromKeyPress(unsigned key_code,
                                       const TCHAR *keys) noexcept
{
  static unsigned keyIdx = 0;
  static unsigned last_key_code = 0;
  static timeval timeOfLastKeyCode;
  timeval now;
  gettimeofday(&now, NULL);
  long microsNow = now.tv_sec * 1000000 + now.tv_usec;
  long microsTimeOfLastKeyCode = timeOfLastKeyCode.tv_sec * 1000000
      + timeOfLastKeyCode.tv_usec;
  TCHAR buffer[MAX_TEXTENTRY];
//  UnsafeCopyString(buffer, dataField.GetAsString());
  if (KEY_BACK == key_code) {
    buffer[StringLength(buffer) - 1] = '\0';
  } else if (last_key_code != key_code
      || ((microsNow - microsTimeOfLastKeyCode) >= waitForSameKeyTime)) {
    keyIdx = 0;
    if (StringLength(keys) > 0) {
      TCHAR keyBuf[2] = { keys[0], '\0' };
      UnsafeCopyString(buffer + StringLength(buffer), keyBuf);
    }
// If there is only one allowed character, it can be taken without key pressing
    TCHAR theOnlyAllowedChar[2] = { '\0', '\0' };
    while (0 != (theOnlyAllowedChar[0] = UpdateAllowedCharacters()))
      UnsafeCopyString(buffer + StringLength(buffer), theOnlyAllowedChar);
  } else {
    keyIdx++;
    if (keyIdx >= StringLength(keys))
      keyIdx = 0;
    buffer[StringLength(buffer) - 1] = keys[keyIdx];
  }
  gettimeofday(&timeOfLastKeyCode, NULL);
  last_key_code = key_code;
//  if (IsDefined())
//    numPad.GetWindow().Invalidate();
}

bool
TextNumPadAdapter::KeyPress(unsigned key_code) noexcept
{
  static const unsigned keyIdxMap[][2] { { KEY_KP0, 9 }, { KEY_KP1, 8 }, {
      KEY_KP2, 7 },
                                         { KEY_KP3, 6 }, { KEY_KP4, 5 }, {
                                             KEY_KP5, 4 },
                                         { KEY_KP6, 3 }, { KEY_KP7, 2 }, {
                                             KEY_KP8, 1 },
                                         { KEY_KP9, 0 },
#if defined(USE_X11)
                                           { KEY_KPHOME, 2 }, {
                                               KEY_KPUP, 1 },
                                           { KEY_PAGEUP, 0 },
                                           { KEY_KPLEFT, 5 },
                                           { KEY_BEGIN, 4 },
                                           { KEY_KPRIGHT, 3 }, { KEY_KPEND, 8 },
                                           { KEY_KPDOWN, 7 }, { KEY_PAGEDOWN,
                                                                 6 },
                                           { KEY_INSERT, 9 },
  #endif
                                         { KEY_BACK, 0 }// Dummy entry for backspace
  };
  if (!numPad->HasFocus())
    return false;
  if(UIGlobals::GetDialogSettings().text_input_style
      == DialogSettings::TextInputStyle::NumPad )
  {
    for (unsigned i = 0; i < sizeof(keyIdxMap) / sizeof(keyIdxMap[0]); i++)
      if (keyIdxMap[i][0] == key_code) {
        setCharFromKeyPress(key_code, allowedCharsForKey[keyIdxMap[i][1]]);
        return true;
      }
    return false;
  }
  return false;
}
