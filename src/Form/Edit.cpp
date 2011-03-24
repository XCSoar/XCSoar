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
#include "DataField/Base.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Key.h"
#include "Screen/Features.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "resource.h"

#include <assert.h>

bool
WndProperty::Editor::on_mouse_down(int x, int y)
{
  // if it's an Combopicker field
  if (parent->mDialogStyle)
    // call the combopicker routine
    if (parent->on_mouse_down(x, y))
      return true;

#ifndef ENABLE_SDL

  // If the Control is read-only -> drop this event,
  // so the default handler doesn't obtain the focus
  if (is_read_only())
    return true;

#endif

  return false;
}

bool
WndProperty::Editor::on_key_check(unsigned key_code) const
{
  switch (key_code) {
  case VK_RETURN:
    return parent->mDialogStyle;

  case VK_LEFT:
  case VK_RIGHT:
    return true;

  default:
    return false;
  }
}

bool
WndProperty::Editor::on_key_down(unsigned key_code)
{
  // If return key pressed (Compaq uses VKF23)
  if (key_code == VK_RETURN)
    // if it's an Combopicker field
    if (parent->mDialogStyle)
      // call the combopicker routine
      if (parent->on_mouse_down(0, 0))
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

  return false;
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

Bitmap WndProperty::hBmpLeft32;
Bitmap WndProperty::hBmpRight32;

int WndProperty::InstCount = 0;

WndProperty::WndProperty(ContainerWindow &parent,
                         const TCHAR *Caption,
                         int X, int Y,
                         int Width, int Height,
                         int CaptionWidth,
                         Color _background_color,
                         const WindowStyle style,
                         const EditWindowStyle edit_style,
                         DataChangeCallback_t DataChangeNotify)
  :mDialogStyle(true), edit(this),
   mBitmapSize(Layout::Scale(32) / 2), mCaptionWidth(CaptionWidth),
   mDownDown(false), mUpDown(false),
   mOnDataChangeNotify(DataChangeNotify),
   mOnClickUpNotify(NULL), mOnClickDownNotify(NULL),
   mDataField(NULL)
{
  _tcscpy(mCaption, Caption);

  set(parent, X, Y, Width, Height, style);

  edit.set(*this, mEditPos.x, mEditPos.y, mEditSize.x, mEditSize.y, edit_style);
  edit.install_wndproc();

  edit.set_font(*GetFont());

#if !defined(ENABLE_SDL) && !defined(NDEBUG)
  ::SetWindowText(hWnd, Caption);
#endif

  SetBackColor(_background_color);

  if (InstCount == 0) {
    hBmpLeft32.load(IDB_DLGBUTTONLEFT32);
    hBmpRight32.load(IDB_DLGBUTTONRIGHT32);
  }

  InstCount++;
}

WndProperty::~WndProperty(void)
{
  InstCount--;
  if (InstCount == 0) {
    hBmpLeft32.reset();
    hBmpRight32.reset();
  }

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
WndProperty::SetText(const TCHAR *Value)
{
  edit.set_text(Value);
}

void
WndProperty::SetFont(const Font &Value)
{
  WindowControl::SetFont(Value);
  edit.set_font(Value);
}

void
WndProperty::BeginEditing()
{
  if (edit.is_read_only()) {
    /* this would display xml file help on a read-only wndproperty if
       it exists */
    OnHelp();
  } else if (mDialogStyle) {
    SingleWindow *root = (SingleWindow *)get_root_owner();

    /* if this asserton fails, then there no valid root window could
       be found - maybe it didn't register its wndproc? */
    assert(root != NULL);

    dlgComboPicker(*root, this);
  } else {
    edit.set_focus();
  }
}

void
WndProperty::UpdateLayout()
{
  mBitmapSize = mDialogStyle || edit.is_read_only() ? 0 : Layout::Scale(16);

  const PixelSize size = get_size();

  if (mCaptionWidth != 0) {
    mEditSize.x = size.cx - mCaptionWidth - (DEFAULTBORDERPENWIDTH + 1)
        - mBitmapSize;
    mEditSize.y = size.cy - 2 * (DEFAULTBORDERPENWIDTH + 1);
    mEditPos.x = mCaptionWidth;
    mEditPos.y = (DEFAULTBORDERPENWIDTH + 1);
  } else {
    mEditSize.x = size.cx - 2 * (DEFAULTBORDERPENWIDTH + 1 + mBitmapSize);
    mEditSize.y = size.cy / 2;
    mEditPos.x = mBitmapSize + (DEFAULTBORDERPENWIDTH + 2);
    mEditPos.y = size.cy / 2 - 2 * (DEFAULTBORDERPENWIDTH + 1);
  }

  mHitRectDown.left = mEditPos.x - mBitmapSize;
  mHitRectDown.top = mEditPos.y + (mEditSize.y) / 2 - (mBitmapSize / 2);
  mHitRectDown.right = mHitRectDown.left + mBitmapSize;
  mHitRectDown.bottom = mHitRectDown.top + mBitmapSize;

  mHitRectUp.left = size.cx - (mBitmapSize + 2);
  mHitRectUp.top = mHitRectDown.top;
  mHitRectUp.right = mHitRectUp.left + mBitmapSize;
  mHitRectUp.bottom = mHitRectUp.top + mBitmapSize;

  if (edit.defined())
    edit.move(mEditPos.x, mEditPos.y, mEditSize.x, mEditSize.y);

  invalidate();
}

void
WndProperty::on_editor_setfocus()
{
  if (mDataField != NULL) {
    edit.set_text(mDataField->GetAsString());
  }

  invalidate();
}

void
WndProperty::on_editor_killfocus()
{
  if (mDataField != NULL) {
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
  if (mDialogStyle) {
    BeginEditing();
  } else {
    if (!edit.has_focus()) {
      if (!edit.is_read_only())
        edit.set_focus();

      return true;
    }

    RasterPoint Pos;
    Pos.x = x;
    Pos.y = y;
    //POINTSTOPOINT(Pos, MAKEPOINTS(lParam));

    mDownDown = (PtInRect(&mHitRectDown, Pos) != 0);
    if (mDownDown) {
      DecValue();
      invalidate(mHitRectDown);
    }

    mUpDown = (PtInRect(&mHitRectUp, Pos) != 0);
    if (mUpDown) {
      IncValue();
      invalidate(mHitRectUp);
    }

    set_capture();
  }

  return true;
}

bool
WndProperty::on_mouse_up(int x, int y)
{
  if (mDialogStyle) {
  } else {
    if (mDownDown) {
      mDownDown = false;
      invalidate(mHitRectDown);
    }
    if (mUpDown) {
      mUpDown = false;
      invalidate(mHitRectUp);
    }
  }
  release_capture();
  return true;
}

int
WndProperty::CallSpecial(void)
{
  if (mDataField != NULL) {
    mDataField->Special();
    edit.set_text(mDataField->GetAsString());
  }
  return 0;
}

int
WndProperty::IncValue(void)
{
  if (mDataField != NULL) {
    mDataField->Inc();
    edit.set_text(mDataField->GetAsString());
  }
  return 0;
}

int
WndProperty::DecValue(void)
{
  if (mDataField != NULL) {
    mDataField->Dec();
    edit.set_text(mDataField->GetAsString());
  }
  return 0;
}

void
WndProperty::on_paint(Canvas &canvas)
{
  /* background and selector */
  if (edit.has_focus()) {
    canvas.clear(GetBackColor().highlight());
    PaintSelector(canvas, get_client_rect());
  } else {
    /* don't need to erase the background when it has been done by the
       parent window already */
#ifdef HAVE_CLIPPING
    canvas.clear(GetBackColor());
#endif
  }

  RasterPoint org;

  WindowControl::on_paint(canvas);

  canvas.set_text_color(GetForeColor());
  canvas.background_transparent();
  canvas.select(*GetFont());

  PixelSize tsize = canvas.text_size(mCaption);

  if (mCaptionWidth == 0) {
    org.x = mEditPos.x;
    org.y = mEditPos.y - tsize.cy;
  } else {
    org.x = mCaptionWidth - mBitmapSize - (tsize.cx + 1);
    org.y = (get_size().cy - tsize.cy) / 2;
  }

  if (org.x < 1)
    org.x = 1;

  canvas.text(org.x, org.y, mCaption);

  // can't but dlgComboPicker here b/c it calls paint when combopicker closes too
  // so it calls dlgCombopicker on the click/focus handlers for the wndproperty & label
  if (!mDialogStyle && edit.has_focus() && !edit.is_read_only()) {
    canvas.stretch(mHitRectDown.left, mHitRectDown.top,
                   mBitmapSize, mBitmapSize,
                   hBmpLeft32,
                   mDownDown ? 32 : 0, 0, 32, 32);

    canvas.stretch(mHitRectUp.left, mHitRectUp.top,
                   mBitmapSize, mBitmapSize,
                   hBmpRight32,
                   mUpDown ? 32 : 0, 0, 32, 32);
  }
}

void
WndProperty::RefreshDisplay()
{
  if (!mDataField)
    return;

  if (edit.has_focus())
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

    mDialogStyle = has_pointer() && mDataField->SupportCombo;

    UpdateLayout();

    RefreshDisplay();
  }

  return res;
}
