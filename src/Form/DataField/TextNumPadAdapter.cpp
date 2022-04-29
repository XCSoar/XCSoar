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
#include "Form/DataField/String.hpp"
#include "LogFile.hpp"

static constexpr TCHAR charsForKey[MAX_BUTTONS][MAX_CHARS_PER_KEY] = {
    _T("ABC1"), _T("DEF2"), _T("GHI3"), _T("JKL4"), _T("MNO5"), _T("PQR6"), _T(
        "STU7"),
    _T("VWX8"), _T("YZ9"), _T(" -0") };

TCHAR allowedCharsForKey[10][5];

TextNumPadAdapter::TextNumPadAdapter(NumPadWidgetInterface *_numPadWidget,
                                     AllowedCharacters acb,
                                     bool _show_shift_button,
                                     bool _default_shift_state) : NumPadAdapter(
    _numPadWidget), NumPadAllowedCharactersCallback(acb), on_character(nullptr), shift_state(
    _default_shift_state), show_shift_button(_show_shift_button), keyPressedTimer(
    std::bind(&TextNumPadAdapter::KeyFinished, this)), selectedButtonIndex(0), numPadEditingActive(
    false){};

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

void
TextNumPadAdapter::SetCaption(unsigned buttonIndex,
                              const TCHAR *allowedChars) const noexcept
{
  unsigned row = buttonIndex / 3;
  unsigned col = buttonIndex % 3;
  TCHAR caption[7] = _T("0\n");
  if (buttonIndex < 9)
    caption[0] = '0' + (2 - row) * 3 + col + 1;
  else
    caption[0] = '0';
  UnsafeCopyString(caption + StringLength(caption), allowedChars);
  TextButtonRenderer &renderer = (TextButtonRenderer&)numPad->GetButtons()[buttonIndex].GetRenderer();
  numPad->GetButtons()[buttonIndex].SetVisible(StringLength(caption) >= 3);
  renderer.SetCaption(caption);
  numPad->GetButtons()->Invalidate();

}

void
TextNumPadAdapter::UpdateAllowedCharacters(
    const TCHAR *dataFieldContent) noexcept
{
  const TCHAR *allowed = nullptr;
  if (NumPadAllowedCharactersCallback)
    allowed = NumPadAllowedCharactersCallback(dataFieldContent);
  if (allowed == nullptr || StringLength(allowed) == 0) {
    for (unsigned i = 0; i < numPad->GetNumButtons(); ++i) {
      if (NumPadAllowedCharactersCallback)
        allowedCharsForKey[i][0] = '\0';
      numPad->GetButtons()[i].SetVisible(false);
      previousButtonIndex = NO_PREVIOUSBUTTON;
    }
    return;
  }
  for (unsigned i = 0; i < numPad->GetNumButtons(); ++i) {
    allowedCharsForKey[i][0] = '\0';
    for (unsigned c = 0; c < StringLength(charsForKey[i]); c++) {
      {
        CheckKey(allowedCharsForKey[i], allowed, charsForKey[i][c]);
      }
    }
  }

// If there is one empty button before or after a button with
// two keys, distribute the characters to this button
// A button with one character needs no wait time for double clicks
  for (unsigned i = 0; i < numPad->GetNumButtons() - 1 ; ++i) {
    if (StringLength(allowedCharsForKey[i]) == 2) {
      if (i > 0 && StringLength(allowedCharsForKey[i - 1]) == 0) {
        allowedCharsForKey[i - 1][0] = allowedCharsForKey[i][1];
        allowedCharsForKey[i - 1][1] = '\0';
        allowedCharsForKey[i][1] = '\0';
      } else if (i < numPad->GetNumButtons() - 2
          && StringLength(allowedCharsForKey[i + 1]) == 0) {
        allowedCharsForKey[i + 1][0] = allowedCharsForKey[i][1];
        allowedCharsForKey[i + 1][1] = '\0';
        allowedCharsForKey[i][1] = '\0';
      }
    }
  }
  for (unsigned i = 0; i < numPad->GetNumButtons(); ++i)
  {
    SetCaption(i, allowedCharsForKey[i]);
  }
}
void
TextNumPadAdapter::UpdateButtons() noexcept
{
  const TCHAR *dataFieldContent = dataField->GetAsString();
  static TCHAR lastAllowedCharacters[256] = _T("");
  if( 0 == StringCompare(lastAllowedCharacters, dataFieldContent) && *dataFieldContent != '\0' )
  {
    return; // nothing to do
  }
  UnsafeCopyString(lastAllowedCharacters, dataFieldContent);
  UpdateAllowedCharacters(dataFieldContent);
  if (!numPadEditingActive && selectedButtonIndex < numPad->GetNumButtons())
  {
    if(!numPad->GetButtons()[selectedButtonIndex].IsVisible())
     SelectNextButton();
    numPad->GetButtons()[selectedButtonIndex].SetSelected(true);
  }
  if (refreshEditFieldFunction)
    this->refreshEditFieldFunction();
}
void
TextNumPadAdapter::OnKeyBack() noexcept
{
  const TCHAR *newAllowedCharacters;
  const TCHAR *dataFieldValue = dataField->GetAsString();
  if (StringLength(dataFieldValue) >= MAX_TEXTENTRY) {
    LogFormat(_T("String too long for TextNumPadAdapter \"%s\""),
              dataFieldValue);
    return;
  }
  TCHAR buffer[MAX_TEXTENTRY];
  UnsafeCopyString(buffer, dataFieldValue);
  do {
    buffer[StringLength(buffer) - 1] = '\0';
    newAllowedCharacters = GetAllowedCharacters(buffer);
  } while (newAllowedCharacters != nullptr
      && StringLength(newAllowedCharacters) == 1 && StringLength(buffer) > 0);
  ((DataFieldString*)dataField)->ModifyValue(buffer);
  previousButtonIndex = NO_PREVIOUSBUTTON;
  UpdateButtons();
}

void
TextNumPadAdapter::SetCharFromKeyPress(
    const TCHAR *allowedCharactersForCurrentKey) noexcept
{
  const TCHAR *dataFieldValue = dataField->GetAsString();
  if (StringLength(dataFieldValue) >= MAX_TEXTENTRY) {
    LogFormat(_T("String too long for TextNumPadAdapter \"%s\""),
              dataFieldValue);
    return;
  }
  TCHAR buffer[MAX_TEXTENTRY];
  UnsafeCopyString(buffer, dataFieldValue);
  if (previousButtonIndex != NO_PREVIOUSBUTTON)// Previous keys active (no timeout and no different key )
      {
// Overwrite the last character of the dataField
    buffer[StringLength(buffer) - 1] = allowedCharactersForCurrentKey[previousKeyIndex++];
    if (previousKeyIndex >= StringLength(allowedCharactersForCurrentKey))
      previousKeyIndex = 0;
  } else {
// Append a new character to the dataField
    TCHAR theOnlyAllowedChar[2] = {
        allowedCharactersForCurrentKey[previousKeyIndex++], '\0' };
    UnsafeCopyString(buffer + StringLength(buffer), theOnlyAllowedChar);
    // If there is only one allowed character, it can be taken without key pressing
      const TCHAR *newAllowedCharacters = GetAllowedCharacters(buffer);
      while (newAllowedCharacters != nullptr
          && StringLength(newAllowedCharacters) == 1
          && StringLength(buffer) < MAX_TEXTENTRY - 1) {
        UnsafeCopyString(buffer + StringLength(buffer), newAllowedCharacters);
        newAllowedCharacters = GetAllowedCharacters(buffer);
      }
    // No Characters left
      if (newAllowedCharacters == nullptr || StringLength(newAllowedCharacters) == 0)
        OnNewKey();
  }
  // Make sure a button is selected

  ((DataFieldString*)dataField)->ModifyValue(buffer);
  if (refreshEditFieldFunction)
    this->refreshEditFieldFunction();
}

static const unsigned keyIdxMap[][2] {
    { KEY_KP0, 9 }, { KEY_KP1, 6 }, { KEY_KP2, 7 }, { KEY_KP3, 8 },
    { KEY_KP4, 3 }, { KEY_KP5, 4 }, { KEY_KP6, 5 }, { KEY_KP7, 0 },
    { KEY_KP8, 1 }, { KEY_KP9, 2 },

#if defined(USE_X11)
                                         { KEY_KPHOME, 0 }, 
                                         { KEY_KPUP, 1 },
                                         { KEY_KPLEFT, 3 },
                                         { KEY_BEGIN, 4 },
                                         { KEY_KPRIGHT, 5 }, 
                                         { KEY_KPEND, 6 },
                                         { KEY_KPDOWN, 7 },
#endif
                                         { KEY_PAGEDOWN, 8 },
                                         { KEY_PAGEUP, 2 },
                                         { KEY_INSERT, 9 },
                                         { KEY_DELETE, NumPadWidgetInterface::EDIT_INDEX },
    { KEY_LEFT, 3 },
    { KEY_RIGHT, 5 }, { KEY_UP, 1 }, { KEY_DOWN, 7 }, { KEY_RETURN, 7 }, {
        KEY_BACK, NumPadWidgetInterface::BACKSPACE_INDEX },
    { KEY_RETURN, NumPadWidgetInterface::EDIT_INDEX }, {
        KEY_KPCOMMA, NumPadWidgetInterface::EDIT_INDEX }, };

bool
TextNumPadAdapter::OnKeyCheck(unsigned key_code) const noexcept
{

  if (numPadEditingActive) {
    for (unsigned i = 0; i < sizeof(keyIdxMap) / sizeof(keyIdxMap[0]); i++)
      if (keyIdxMap[i][0] == key_code)
        return true;
  } else
    switch (key_code) {
    case KEY_RIGHT:
    case KEY_LEFT:
#ifdef USE_X11
    case KEY_KPCOMMA:
#endif
    case KEY_DELETE:
    case KEY_RETURN:
      return true;
    }
  return false;
}

bool
TextNumPadAdapter::OnKeyDown(unsigned key_code) noexcept
{
// This will work for all keyboardtypes (some of them
// have no Backspace
  if (key_code == KEY_BACK)
    OnKeyBack();

  if (numPadEditingActive) {
    switch (key_code) {
    case KEY_RETURN:
      OnKeyEdit();
      return true;

//  case KEY_SHIFT:
//    OnKeyFinish();
//    return true;
    }
    unsigned i = 0;
    for (; i < sizeof(keyIdxMap) / sizeof(keyIdxMap[0]); i++)
      if (keyIdxMap[i][0] == key_code) {
        OnButton(keyIdxMap[i][1]);
        return true;
      }

    if (i == sizeof(keyIdxMap) / sizeof(keyIdxMap[0])) {
      LogFormat("Key not found %x %u %c", key_code, key_code, key_code);
    }
  } else {
    switch (key_code) {
    case KEY_RIGHT:
      SelectNextButton();
      return true;
    case KEY_LEFT:
      SelectPreviousButton();
      return true;
#ifdef USE_X11
    case KEY_KPCOMMA:// Only a NumPad can press this key
#endif
    case KEY_DELETE:
      BeginEditing();
      return true;
    case KEY_RETURN:
// Execute OnButton on the selected key
      OnSelectedButton();
      return true;
    }

  }
  return false;
}

void
TextNumPadAdapter::OnDataFieldSetFocus() noexcept
{
  NumPadAdapter::OnDataFieldSetFocus();
  previousButtonIndex = NO_PREVIOUSBUTTON;
  UpdateButtons();
}

void
TextNumPadAdapter::SelectNextButton() noexcept
{
  if (numPadEditingActive)
    return;// No selection required
  unsigned previousSelectedButtonIndex = selectedButtonIndex;
  if (selectedButtonIndex < numPad->GetNumButtons())
    numPad->GetButtons()[selectedButtonIndex].SetSelected(false);
  unsigned count = 0;
  do {
    if (selectedButtonIndex < numPad->GetNumButtons()) {
      selectedButtonIndex++;
    } else
      selectedButtonIndex = 0;// no selected button select the first one

    if (selectedButtonIndex == numPad->GetNumButtons())
      selectedButtonIndex = 0;// wrap around
    count++;
  } while (count < numPad->GetNumButtons()
      && selectedButtonIndex != previousSelectedButtonIndex
      && allowedCharsForKey[selectedButtonIndex][0] == '\0');

  numPad->GetButtons()[selectedButtonIndex].SetSelected(true);
}
void
TextNumPadAdapter::SelectPreviousButton() noexcept
{
  unsigned previousSelectedButtonIndex = selectedButtonIndex;
  if (numPadEditingActive)
    return;// No selection required
  if (selectedButtonIndex < numPad->GetNumButtons())
    numPad->GetButtons()[selectedButtonIndex].SetSelected(false);
  else
    selectedButtonIndex = 1;// will be decremented

  unsigned count = 0;
  do {

    if (selectedButtonIndex > 0)
      selectedButtonIndex--;
    else
// wrap around
      selectedButtonIndex = numPad->GetNumButtons() - 1;
    count++;
  } while (count < numPad->GetNumButtons()
      && selectedButtonIndex != previousSelectedButtonIndex
      && allowedCharsForKey[selectedButtonIndex][0] == '\0');

  numPad->GetButtons()[selectedButtonIndex].SetSelected(true);
}
void
TextNumPadAdapter::OnSelectedButton() noexcept
{
  if (selectedButtonIndex < numPad->GetNumButtons())
    OnButton(selectedButtonIndex);// Button remains selected
}

void
TextNumPadAdapter::BeginEditing() noexcept
{
  numPadEditingActive = true;
  if (selectedButtonIndex < numPad->GetNumButtons()) {
    numPad->GetButtons()[selectedButtonIndex].SetSelected(false);
    selectedButtonIndex = numPad->GetNumButtons();
  }

  NumPadAdapter::BeginEditing();
  UpdateButtons();
  previousButtonIndex = NO_PREVIOUSBUTTON;
}
void
TextNumPadAdapter::EndEditing() noexcept
{
  numPadEditingActive = false;
  NumPadAdapter::EndEditing();
  selectedButtonIndex = 0;
  for (;
      selectedButtonIndex < numPad->GetNumButtons()
          && !numPad->GetButtons()[selectedButtonIndex].IsVisible();
      selectedButtonIndex++);

  if(selectedButtonIndex < numPad->GetNumButtons())
    numPad->GetButtons()[selectedButtonIndex].SetSelected(true);

  UpdateButtons();
}
/*
 *  User pressed a different key
 */
void
TextNumPadAdapter::OnNewKey() noexcept
{
  previousButtonIndex = NO_PREVIOUSBUTTON;
  previousKeyIndex = 0;
  UpdateButtons();
  keyPressedTimer.Cancel();
}
/*
 *  User stopped pressing the same key
 */
void
TextNumPadAdapter::KeyFinished() noexcept
{
  previousButtonIndex = NO_PREVIOUSBUTTON;
  previousKeyIndex = 0;
  UpdateButtons();
  keyPressedTimer.Cancel();
}

bool
TextNumPadAdapter::CharacterFunction(unsigned ch) noexcept
{
  if (ch == 8)// Handle backspace, because we might not have the focus
      {
    OnKeyBack();
    return true;
  }
  if (ch < 32 || ch > 255)// ignore special characters
    return false;
  const TCHAR *newAllowedCharacters;
  const TCHAR *dataFieldValue = dataField->GetAsString();
  if (StringLength(dataFieldValue) >= MAX_TEXTENTRY) {
    LogFormat(_T("String too long for TextNumPadAdapter \"%s\""),
              dataFieldValue);
    return false;
  }
  if (shift_state)
    ch = ToUpperASCII((TCHAR)ch);
  TCHAR buffer[MAX_TEXTENTRY];
  UnsafeCopyString(buffer, dataFieldValue);
  unsigned oldLength = StringLength(buffer);
  buffer[oldLength] = ch;
  buffer[oldLength+1] = '\0';
  newAllowedCharacters = GetAllowedCharacters(buffer);
  if(newAllowedCharacters == nullptr || *newAllowedCharacters== '\0' )
  {
    UpdateButtons();
    return false;
  }
  ((DataFieldString*)dataField)->ModifyValue(buffer);
  if (refreshEditFieldFunction)
    this->refreshEditFieldFunction();
  OnNewKey();
  return true;
}

const TCHAR*
TextNumPadAdapter::GetAllowedCharacters(const TCHAR *prefix) noexcept
{
  if (NumPadAllowedCharactersCallback)
    return NumPadAllowedCharactersCallback(prefix);
  return nullptr;
}

void
TextNumPadAdapter::OnKeyEdit() noexcept
{
  EndEditing();
}

void
TextNumPadAdapter::OnButton(unsigned buttonIndex) noexcept
{
  switch (buttonIndex) {
  case NumPadWidgetInterface::SHIFT_INDEX:
    break;
  case NumPadWidgetInterface::BACKSPACE_INDEX:
    OnKeyBack();
    break;
  case NumPadWidgetInterface::EDIT_INDEX:
    OnKeyEdit();
    break;
  default:
    assert(buttonIndex <= GetNumPadWidgetInterface()->GetNumButtons());
// if the buttons differs from the previous one, the previous one is valid
    if (previousButtonIndex != NO_PREVIOUSBUTTON && previousButtonIndex != buttonIndex)
      OnNewKey();
    SetCharFromKeyPress(allowedCharsForKey[buttonIndex]);

// If the button has only one valid character, we don't need to wait
// for double clicks
    if (StringLength(allowedCharsForKey[buttonIndex]) > 1) {
      keyPressedTimer.Schedule(TIMEOUT_FOR_KEYPRESSED);
      previousButtonIndex = buttonIndex;
    } else
      OnNewKey();
    break;
  }

}

unsigned
TextNumPadAdapter::GetButtonIndex(unsigned row, unsigned column) const noexcept
{
  if (row <= 3)
    return row * 3 + column;
  switch (column) {
  case 0:
    return NumPadWidgetInterface::SHIFT_INDEX;
  case 1:
    return NumPadWidgetInterface::BACKSPACE_INDEX;
  case 2:
    return NumPadWidgetInterface::EDIT_INDEX;
  }
  return 0;
}

unsigned
TextNumPadAdapter::GetRowFromButtonIndex(unsigned buttonIndex) const noexcept
{
  if (buttonIndex < 10)
    return buttonIndex / 3;
  return 4;
}
unsigned
TextNumPadAdapter::GetColumnFromButtonIndex(unsigned buttonIndex) const noexcept
{
  if (buttonIndex < 10)
    return buttonIndex % 3;
  switch (buttonIndex) {
  case NumPadWidgetInterface::SHIFT_INDEX:
    return 0;
  case NumPadWidgetInterface::BACKSPACE_INDEX:
    return 1;
  case NumPadWidgetInterface::EDIT_INDEX:
    return 2;
  }
  return 0;
}

