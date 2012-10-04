/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
     return HasKeyboard(); */
  return false;
}

bool
WndProperty::Editor::OnMouseDown(PixelScalar x, PixelScalar y)
{
  if (parent->OnMouseDown(x, y))
    return true;

#ifdef USE_GDI

  // If the Control is read-only -> drop this event,
  // so the default handler doesn't obtain the focus
  if (IsReadOnly())
    return true;

#endif

  return EditWindow::OnMouseDown(x, y);
}

bool
WndProperty::Editor::OnKeyCheck(unsigned key_code) const
{
  switch (key_code) {
  case VK_RETURN:
    return IsReadOnly() ||
      (parent->mDataField != NULL && parent->mDataField->supports_combolist) ||
      !CanEditInPlace() || parent->HasHelp();

  case VK_LEFT:
  case VK_RIGHT:
    return !IsReadOnly();

  default:
    return EditWindow::OnKeyCheck(key_code);
  }
}

bool
WndProperty::Editor::OnKeyDown(unsigned key_code)
{
  // If return key pressed (Compaq uses VKF23)
  if (key_code == VK_RETURN && parent->OnMouseDown(0, 0))
    return true;

  switch (key_code) {
  case VK_RIGHT:
    if (IsReadOnly())
      break;

    parent->IncValue();
    return true;
  case VK_LEFT:
    if (IsReadOnly())
      break;

    parent->DecValue();
    return true;
  }

  KeyTimer(true, key_code);
  return EditWindow::OnKeyDown(key_code);
}

bool
WndProperty::Editor::OnKeyUp(unsigned key_code)
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

  return EditWindow::OnKeyUp(key_code);
}

void
WndProperty::Editor::OnSetFocus()
{
  KeyTimer(true, 0);
  EditWindow::OnSetFocus();
  parent->on_editor_setfocus();
  SelectAll();
}

void
WndProperty::Editor::OnKillFocus()
{
  KeyTimer(true, 0);
  parent->on_editor_killfocus();
  EditWindow::OnKillFocus();
}

WndProperty::WndProperty(ContainerWindow &parent, const DialogLook &_look,
                         const TCHAR *Caption,
                         const PixelRect &rc,
                         int CaptionWidth,
                         const WindowStyle style,
                         const EditWindowStyle edit_style,
                         DataChangeCallback_t DataChangeNotify)
  :look(_look), edit(this),
   caption_width(CaptionWidth),
   mOnDataChangeNotify(DataChangeNotify),
   mDataField(NULL)
{
  caption = Caption;

  Create(parent, rc, style);

  edit.Create(*this, edit_rc, edit_style);
  edit.InstallWndProc();

  edit.SetFont(*look.text_font);

#if defined(USE_GDI) && !defined(NDEBUG)
  ::SetWindowText(hWnd, Caption);
#endif
}

WndProperty::~WndProperty()
{
  delete mDataField;
}

UPixelScalar
WndProperty::GetRecommendedCaptionWidth() const
{
  return look.text_font->TextSize(caption).cx + Layout::FastScale(3);
}

void
WndProperty::SetCaptionWidth(PixelScalar _caption_width)
{
  if (caption_width == _caption_width)
    return;

  caption_width = _caption_width;
  UpdateLayout();
}

bool
WndProperty::BeginEditing()
{
  if (edit.IsReadOnly()) {
    /* this would display xml file help on a read-only wndproperty if
       it exists */
    return OnHelp();
  } else if (mDataField != NULL && mDataField->supports_combolist) {
    SingleWindow *root = (SingleWindow *)GetRootOwner();

    /* if this asserton fails, then there no valid root window could
       be found - maybe it didn't register its wndproc? */
    assert(root != NULL);

    dlgComboPicker(*root, this);
    return true;
  } else if (CanEditInPlace()) {
    edit.SetFocus();
    return true;
  } else if (mDataField != NULL) {
    const TCHAR *value = mDataField->GetAsString();
    if (value == NULL)
      return false;

    StaticString<EDITSTRINGSIZE> buffer(value);
    if (!TextEntryDialog(*(SingleWindow *)GetRootOwner(), buffer,
                         GetCaption()))
      return true;

    mDataField->SetAsString(buffer);
    RefreshDisplay();
    return true;
  } else
    return false;
}

void
WndProperty::UpdateLayout()
{
  edit_rc = GetClientRect();

  const UPixelScalar DEFAULTBORDERPENWIDTH = Layout::FastScale(1);

  if (caption_width >= 0) {
    edit_rc.left += caption_width + (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.top += (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.right -= (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.bottom -= (DEFAULTBORDERPENWIDTH + 1);
  } else {
    edit_rc.left += (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.top = (edit_rc.top + edit_rc.bottom) / 2
      - 2 * (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.right -= (DEFAULTBORDERPENWIDTH + 1);
    edit_rc.bottom -= (DEFAULTBORDERPENWIDTH + 1);
  }

  if (edit.IsDefined())
    edit.Move(edit_rc);

  Invalidate();
}

void
WndProperty::on_editor_setfocus()
{
  if (mDataField != NULL && CanEditInPlace()) {
    edit.SetText(mDataField->GetAsString());
  }

  Invalidate();
}

void
WndProperty::on_editor_killfocus()
{
  if (mDataField != NULL && CanEditInPlace()) {
    TCHAR sTmp[128];
    edit.GetText(sTmp, (sizeof(sTmp) / sizeof(TCHAR)) - 1);
    mDataField->SetAsString(sTmp);
    edit.SetText(mDataField->GetAsDisplayString());
  }

  Invalidate();
}

void
WndProperty::OnResize(UPixelScalar width, UPixelScalar height)
{
  WindowControl::OnResize(width, height);
  UpdateLayout();
}

bool
WndProperty::OnMouseDown(PixelScalar x, PixelScalar y)
{
  return BeginEditing();
}

bool
WndProperty::OnMouseUp(PixelScalar x, PixelScalar y)
{
  return true;
}

int
WndProperty::CallSpecial()
{
  if (mDataField != NULL) {
    mDataField->Special();
    RefreshDisplay();
  }
  return 0;
}

int
WndProperty::IncValue()
{
  if (mDataField != NULL) {
    mDataField->Inc();
    RefreshDisplay();
  }
  return 0;
}

int
WndProperty::DecValue()
{
  if (mDataField != NULL) {
    mDataField->Dec();
    RefreshDisplay();
  }
  return 0;
}

void
WndProperty::OnPaint(Canvas &canvas)
{
  const bool focused = edit.HasFocus();

  /* background and selector */
  if (focused) {
    canvas.Clear(look.focused.background_color);
  } else {
    /* don't need to erase the background when it has been done by the
       parent window already */
    if (HaveClipping())
      canvas.Clear(look.background_color);
  }

  WindowControl::OnPaint(canvas);

  /* kludge: don't draw caption if width is too small (but not 0),
     used by the polar configuration panel.  This concept needs to be
     redesigned. */
  if (caption_width != 0 && !caption.empty()) {
    canvas.SetTextColor(focused
                          ? look.focused.text_color
                          : look.text_color);
    canvas.SetBackgroundTransparent();
    canvas.Select(*look.text_font);

    PixelSize tsize = canvas.CalcTextSize(caption.c_str());

    RasterPoint org;
    if (caption_width < 0) {
      org.x = edit_rc.left;
      org.y = edit_rc.top - tsize.cy;
    } else {
      org.x = caption_width - tsize.cx - Layout::FastScale(3);
      org.y = (GetHeight() - tsize.cy) / 2;
    }

    if (org.x < 1)
      org.x = 1;

    if (HaveClipping())
      canvas.text(org.x, org.y, caption.c_str());
    else
      canvas.text_clipped(org.x, org.y, caption_width - org.x,
                          caption.c_str());
  }
}

void
WndProperty::RefreshDisplay()
{
  if (!mDataField)
    return;

  if (edit.HasFocus() && CanEditInPlace())
    edit.SetText(mDataField->GetAsString());
  else
    edit.SetText(mDataField->GetAsDisplayString());
}

void
WndProperty::SetDataField(DataField *Value)
{
  assert(mDataField == NULL || mDataField != Value);

  delete mDataField;
  mDataField = Value;

  UpdateLayout();

  RefreshDisplay();
}
