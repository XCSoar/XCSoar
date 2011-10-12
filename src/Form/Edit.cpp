/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Form/Edit.hpp"
#include "Form/Internal.hpp"
#include "Look/DialogLook.hpp"
#include "DataField/Base.hpp"
#include "DataField/String.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Features.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/TextEntry.hpp"
#include "resource.h"

#include <assert.h>

static bool
CanEditInPlace()
{
  /* disabled for now, because we don't handle this yet properly:
     return has_keyboard(); */
  return false;
}

bool
WndProperty::Editor::on_mouse_down(int x, int y)
{
  if (parent->on_mouse_down(x, y))
    return true;

#ifdef USE_GDI

  // If the Control is read-only -> drop this event,
  // so the default handler doesn't obtain the focus
  if (is_read_only())
    return true;

#endif

  return EditWindow::on_mouse_down(x, y);
}

bool
WndProperty::Editor::on_key_check(unsigned key_code) const
{
  switch (key_code) {
  case VK_RETURN:
    return is_read_only() ||
      (parent->mDataField != NULL && parent->mDataField->SupportCombo) ||
      !CanEditInPlace();

  case VK_LEFT:
  case VK_RIGHT:
    return true;

  default:
    return EditWindow::on_key_check(key_code);
  }
}

bool
WndProperty::Editor::on_key_down(unsigned key_code)
{
  // If return key pressed (Compaq uses VKF23)
  if (key_code == VK_RETURN && parent->on_mouse_down(0, 0))
    return true;

  // Check for long key press
  // tmep hack, do not process nav keys
  if (KeyTimer(true, key_code)) {
    // Activate Help dialog
    if (key_code == VK_RETURN) {
      if (parent->OnHelp())
        return true;
    }
  }

  switch (key_code) {
  case VK_RIGHT:
    parent->IncValue();
    return true;
  case VK_LEFT:
    parent->DecValue();
    return true;
  }

  return EditWindow::on_key_down(key_code);
}

bool
WndProperty::Editor::on_key_up(unsigned key_code)
{
  if (KeyTimer(false, key_code)) {
    // activate tool tips if hit return for long time
    if (key_code == VK_RETURN) {
      if (parent->OnHelp())
        return true;
    }
  } else if (key_code == VK_RETURN) {
    if (parent->CallSpecial())
      return true;
  }

  return EditWindow::on_key_up(key_code);
}

bool
WndProperty::Editor::on_setfocus()
{
  KeyTimer(true, 0);
  EditWindow::on_setfocus();
  parent->on_editor_setfocus();
  set_selection();
  return true;
}

bool
WndProperty::Editor::on_killfocus()
{
  KeyTimer(true, 0);
  parent->on_editor_killfocus();
  EditWindow::on_killfocus();
  return true;
}

WndProperty::WndProperty(ContainerWindow &parent, const DialogLook &_look,
                         const TCHAR *Caption,
                         int X, int Y,
                         int Width, int Height,
                         int CaptionWidth,
                         const WindowStyle style,
                         const EditWindowStyle edit_style,
                         DataChangeCallback_t DataChangeNotify)
  :look(_look), edit(this),
   mCaptionWidth(CaptionWidth),
   mOnDataChangeNotify(DataChangeNotify),
   mOnClickUpNotify(NULL), mOnClickDownNotify(NULL),
   mDataField(NULL)
{
  mCaption = Caption;

  set(parent, X, Y, Width, Height, style);

  edit.set(*this, mEditPos.x, mEditPos.y, mEditSize.x, mEditSize.y, edit_style);
  edit.install_wndproc();

  edit.set_font(*look.text_font);

#if defined(USE_GDI) && !defined(NDEBUG)
  ::SetWindowText(hWnd, Caption);
#endif
}

WndProperty::~WndProperty(void)
{
  if (mDataField != NULL) {
    if (!mDataField->Unuse()) {
      delete mDataField;
      mDataField = NULL;
    } else {
      assert(0);
    }
  }
}

void
WndProperty::SetText(const TCHAR *Value, bool convert_line_breaks)
{
#ifdef USE_GDI
  if (!convert_line_breaks) {
    edit.set_text(Value);
    return;
  }

  // Replace \n by \r\r\n to enable usage of line-breaks in edit control
  unsigned size = _tcslen(Value);
  TCHAR buffer[size * sizeof(TCHAR) * 3];
  const TCHAR* p2 = Value;
  TCHAR* p3 = buffer;
  for (;*p2 != _T('\0'); p2++) {
    if (*p2 == _T('\n')) {
      *p3 = _T('\r');
      p3++;
      *p3 = _T('\r');
      p3++;
      *p3 = _T('\n');
    } else if (*p2 == _T('\r')) {
      continue;
    } else {
      *p3 = *p2;
    }
    p3++;
  }
  *p3 = _T('\0');
  edit.set_text(buffer);
#else
  edit.set_text(Value);
#endif
}

void
WndProperty::BeginEditing()
{
  if (edit.is_read_only()) {
    /* this would display xml file help on a read-only wndproperty if
       it exists */
    OnHelp();
  } else if (mDataField != NULL && mDataField->SupportCombo) {
    SingleWindow *root = (SingleWindow *)get_root_owner();

    /* if this asserton fails, then there no valid root window could
       be found - maybe it didn't register its wndproc? */
    assert(root != NULL);

    dlgComboPicker(*root, this);
  } else if (CanEditInPlace()) {
    edit.set_focus();
  } else if (mDataField != NULL) {
    const TCHAR *value = mDataField->GetAsString();
    if (value == NULL)
      return;

    StaticString<EDITSTRINGSIZE> buffer(value);
    if (!TextEntryDialog(*(SingleWindow *)get_root_owner(), buffer,
                         GetCaption()))
      return;

    mDataField->SetAsString(buffer);
    RefreshDisplay();
  }
}

void
WndProperty::UpdateLayout()
{
  const PixelSize size = get_size();

  if (mCaptionWidth >= 0) {
    mEditSize.x = size.cx - mCaptionWidth - (DEFAULTBORDERPENWIDTH + 1)*2;
    mEditSize.y = size.cy - 2 * (DEFAULTBORDERPENWIDTH + 1);
    mEditPos.x = mCaptionWidth + (DEFAULTBORDERPENWIDTH + 1);
    mEditPos.y = (DEFAULTBORDERPENWIDTH + 1);
  } else {
    mEditSize.x = size.cx - 2 * (DEFAULTBORDERPENWIDTH + 1);
    mEditSize.y = size.cy / 2;
    mEditPos.x = (DEFAULTBORDERPENWIDTH + 2);
    mEditPos.y = size.cy / 2 - 2 * (DEFAULTBORDERPENWIDTH + 1);
  }

  if (edit.defined())
    edit.move(mEditPos.x, mEditPos.y, mEditSize.x, mEditSize.y);

  invalidate();
}

void
WndProperty::on_editor_setfocus()
{
  if (mDataField != NULL && CanEditInPlace()) {
    edit.set_text(mDataField->GetAsString());
  }

  invalidate();
}

void
WndProperty::on_editor_killfocus()
{
  if (mDataField != NULL && CanEditInPlace()) {
    TCHAR sTmp[128];
    edit.get_text(sTmp, (sizeof(sTmp) / sizeof(TCHAR)) - 1);
    mDataField->SetAsString(sTmp);
    edit.set_text(mDataField->GetAsDisplayString());
  }

  invalidate();
}

bool
WndProperty::on_resize(unsigned width, unsigned height)
{
  WindowControl::on_resize(width, height);
  UpdateLayout();
  return true;
}

bool
WndProperty::on_mouse_down(int x, int y)
{
  BeginEditing();
  return true;
}

bool
WndProperty::on_mouse_up(int x, int y)
{
  return true;
}

int
WndProperty::CallSpecial(void)
{
  if (mDataField != NULL) {
    mDataField->Special();
    RefreshDisplay();
  }
  return 0;
}

int
WndProperty::IncValue(void)
{
  if (mDataField != NULL) {
    mDataField->Inc();
    RefreshDisplay();
  }
  return 0;
}

int
WndProperty::DecValue(void)
{
  if (mDataField != NULL) {
    mDataField->Dec();
    RefreshDisplay();
  }
  return 0;
}

void
WndProperty::on_paint(Canvas &canvas)
{
  const bool focused = edit.has_focus();

  /* background and selector */
  if (focused) {
    canvas.clear(look.focused.background_color);
    PaintSelector(canvas, get_client_rect(), look);
  } else {
    /* don't need to erase the background when it has been done by the
       parent window already */
    if (have_clipping())
      canvas.clear(look.background_color);
  }

  WindowControl::on_paint(canvas);

  /* kludge: don't draw caption if width is too small (but not 0),
     used by the polar configuration panel.  This concept needs to be
     redesigned. */
  if (mCaptionWidth != 0 && !mCaption.empty()) {
    canvas.set_text_color(focused
                          ? look.focused.text_color
                          : look.text_color);
    canvas.background_transparent();
    canvas.select(*look.text_font);

    PixelSize tsize = canvas.text_size(mCaption.c_str());

    RasterPoint org;
    if (mCaptionWidth < 0) {
      org.x = mEditPos.x;
      org.y = mEditPos.y - tsize.cy;
    } else {
      org.x = mCaptionWidth - (tsize.cx + 1);
      org.y = (get_size().cy - tsize.cy) / 2;
    }

    if (org.x < 1)
      org.x = 1;

    if (have_clipping())
      canvas.text(org.x, org.y, mCaption.c_str());
    else
      canvas.text_clipped(org.x, org.y, mCaptionWidth - org.x,
                          mCaption.c_str());
  }
}

void
WndProperty::RefreshDisplay()
{
  if (!mDataField)
    return;

  if (edit.has_focus() && CanEditInPlace())
    edit.set_text(mDataField->GetAsString());
  else
    edit.set_text(mDataField->GetAsDisplayString());
}

DataField *
WndProperty::SetDataField(DataField *Value)
{
  DataField *res = mDataField;

  if (mDataField != Value) {
    if (mDataField != NULL) {
      if (!mDataField->Unuse()) {
        delete (mDataField);
        res = NULL;
      }
    }

    Value->Use();

    mDataField = Value;

    UpdateLayout();

    RefreshDisplay();
  }

  return res;
}
