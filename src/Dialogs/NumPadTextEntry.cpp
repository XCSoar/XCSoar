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

#include "Dialogs/TextEntry.hpp"
#include "WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "Widget/WindowWidget.hpp"
#include "ui/canvas/Canvas.hpp"
#include "ui/event/KeyCode.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "util/CharUtil.hxx"
#include "util/Macros.hpp"
#include "util/StringStrip.hxx"
#include "util/TruncateString.hpp"
#include "util/TStringView.hxx"
#include "LogFile.hpp"

#include <time.h>
#include <sys/time.h>

#include <stdio.h>

#include <algorithm>

#include <string.h>

enum Buttons
{
  DOWN,
  UP,
  LEFT,
  RIGHT,
};

static constexpr long waitForSameKeyTime = 1000000; // one second = 1000.000 microseconds
static constexpr size_t MAX_TEXTENTRY = 40;
static const TCHAR *charsForKey[] = { " -0", "ABC1", "DEF2", "GHI3", "JKL4",
                                      "MNO5", "PQR6", "STU7", "VWX8", "YZ9" };
static const TCHAR *helpMsg[] = {"Use the number pad to select a character.",
    "For selection click the same key several times.",
    "Wait for one second to start a new character.",
    "Use the escape key to abort and the return key to return."
};


class NumPadTextEntryWindow final : public PaintWindow
{
  const size_t max_width;

  TCHAR buffer[MAX_TEXTENTRY];

public:
  NumPadTextEntryWindow(const TCHAR *text, size_t width) :
      max_width(std::min(MAX_TEXTENTRY, width))
  {
    CopyTruncateString(buffer, max_width, text);
  }

  TCHAR*
  GetValue()
  {
    return buffer;
  }

private:
public:
  bool KeyPress(unsigned key_code) noexcept;

protected:
  /* virtual methods from class Window */
  void
  OnPaint(Canvas &canvas) override;
};

void
NumPadTextEntryWindow::OnPaint(Canvas &canvas)
{
  const PixelRect rc = GetClientRect();

  canvas.Clear(Color(0x40, 0x40, 0x00));

  // Do the actual painting of the text
  const DialogLook &look = UIGlobals::GetDialogLook();
  canvas.Select(look.text_font);

  PixelSize tsize = canvas.CalcTextSize(buffer);
  PixelSize tsizec = canvas.CalcTextSize(  "W");
  PixelSize tsizea = canvas.CalcTextSize( "W");

  BulkPixelPoint p[5];
  p[0].x = 10;
  p[0].y = (rc.GetHeight() - tsize.height - 5) / 2;

  p[2].x = p[0].x + tsizec.width;
  p[2].y = p[0].y + tsize.height + 5;

  p[3].x = p[0].x + tsizea.width;
  p[3].y = p[0].y + tsize.height + 5;

  p[1].x = p[2].x;
  p[1].y = p[2].y - 2;

  p[4].x = p[3].x;
  p[4].y = p[3].y - 2;
  int btnHeight=60;
  int btnWidth=60;
  int distance=5;
  int origX=200;
  int origY=p[0].y - 100;
  int keyIdx=0;
  int keyIdy=3;
  int keyText=1;
  PixelSize ps(3,3);
  canvas.SelectWhitePen();
  canvas.SetBackgroundTransparent();
  canvas.SetTextColor(COLOR_WHITE);

    for( int vert = 0; vert < 3*btnHeight; vert += btnHeight)
    {
      keyIdy--;
      keyIdx=0;
      for( int horiz=0;  horiz < 3* btnWidth;horiz += btnWidth)
    {
      PixelRect r(origX +horiz,origY + vert,origX + horiz + btnWidth - distance,
                  origY + vert + btnHeight - distance);
      canvas.DrawRoundRectangle(r, ps);
      BulkPixelPoint pt(origX +horiz,origY + vert + btnHeight / 2);
      BulkPixelPoint ptNumber(origX +horiz + btnWidth/2,origY + vert + btnHeight / 4);
      canvas.Select(look.bold_font);
      char buf[10];
      sprintf(buf,"%d", 1+keyIdy*3+keyIdx); // keyIdx start with 1
      canvas.DrawText(ptNumber, buf);

      canvas.Select(look.text_font);
      canvas.DrawText(pt, charsForKey[keyText++]);
      keyIdx++;
    }
    }
  PixelRect r(origX ,origY + 3*btnHeight,origX + 2 * btnWidth - distance,
              origY + 4*btnHeight - distance);
  canvas.DrawRoundRectangle(r, ps);
  BulkPixelPoint pt(origX + btnWidth,origY + 7 * btnHeight / 2);
  BulkPixelPoint ptNumber(origX +btnWidth/2 ,origY + 3* btnHeight + btnHeight / 4);
  canvas.Select(look.bold_font);
  canvas.DrawText(ptNumber, "0");
  canvas.Select(look.text_font);
  canvas.DrawText(pt, charsForKey[0]);
  PixelRect rTextField(10 ,origY - btnHeight, 400, origY - 3*btnHeight/2);
  canvas.DrawRoundRectangle(rTextField, ps);

  canvas.DrawPolyline(p + 1, 4);
  BulkPixelPoint ptextField( 15,origY - 3*btnHeight/2+tsize.height/2);

  canvas.DrawText(ptextField, buffer);
  BulkPixelPoint pHelpField( 15,origY + 4*btnHeight);
  BulkPixelPoint pNextLine( 0, 5 * tsize.height/4);
  for(unsigned idx=0; idx < sizeof(helpMsg)/sizeof(helpMsg[0]);idx++)
  {
    canvas.DrawText(pHelpField, gettext(helpMsg[idx]));
    pHelpField=pHelpField + pNextLine;
  }
}

class NumPadTextEntryWidget final : public WindowWidget
{
  const TCHAR *const text;
  const size_t width;
  WidgetDialog *dialog;
  void
  setCharFromKeyPress(unsigned key_code, const TCHAR *keys) noexcept;

public:
  NumPadTextEntryWidget(TCHAR *_text, size_t _width) noexcept :
      text(_text), width(_width)
  {
  }

  auto&
  GetWindow() noexcept
  {
    return (NumPadTextEntryWindow&)WindowWidget::GetWindow();
  }

  TCHAR*
  GetValue()
  {
    return GetWindow().GetValue();
  }

  void
  SetDialog(WidgetDialog &dialog);

  /* virtual methods from class Widget */

  void
  Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override
  {
    WindowStyle style;
    style.Hide();

    auto w = std::make_unique < NumPadTextEntryWindow > (text, width);
    w->Create(parent, rc, style);
    SetWindow(std::move(w));
  }
  bool
  KeyPress(unsigned key_code) noexcept override;
};

inline void
NumPadTextEntryWidget::setCharFromKeyPress(unsigned key_code,
                                           const TCHAR *keys) noexcept
{
  static unsigned keyIdx = 0;
  static unsigned last_key_code = 0;
  static timeval timeOfLastKeyCode;
  timeval now;
  gettimeofday(&now, NULL);
  long microsNow=now.tv_sec* 1000000 + now.tv_usec;
  long microsTimeOfLastKeyCode=timeOfLastKeyCode.tv_sec* 1000000 + timeOfLastKeyCode.tv_usec;
  char *buffer = GetValue();
  if (KEY_BACKSPACE == key_code) {
    buffer[strlen(buffer) - 1] = '\0';
  } else if (last_key_code != key_code
      || ((microsNow - microsTimeOfLastKeyCode) >= waitForSameKeyTime)) {
    keyIdx = 0;
    strncat(buffer, keys, 1);
  } else {
    keyIdx++;
    if (keyIdx >= strlen(keys))
      keyIdx = 0;
    buffer[strlen(buffer) - 1] = keys[keyIdx];
  }

  gettimeofday(&timeOfLastKeyCode, NULL);
  last_key_code = key_code;
  if (IsDefined())
    GetWindow().Invalidate();
}



inline bool
NumPadTextEntryWidget::KeyPress(unsigned key_code) noexcept
{
  static const unsigned keyIdxMap[][2] {
     { KEY_KP0, 9 },
     { KEY_KP1, 8 },
     { KEY_KP2, 7 },
     { KEY_KP3, 6 },
     { KEY_KP4, 5 },
     { KEY_KP5, 4 },
     { KEY_KP6, 3 },
     { KEY_KP7, 2 },
     { KEY_KP8, 1 },
     { KEY_KP9, 0 },
     { KEY_KP_HOME, 1 },
     { KEY_KP_UP, 2 },
     { KEY_KP_PAGE_UP, 3 },
     { KEY_KP_LEFT, 4 },
     { KEY_KP_BEGIN, 5 },
     { KEY_KP_RIGHT, 6 },
     { KEY_KP_END, 7 },
     { KEY_KP_DOWN, 8 },
     { KEY_KP_PAGE_DOWN, 9 },
     { KEY_KP_INSERT, 0 },
     { KEY_BACKSPACE, 0 } // Dummy entry for backspace
  };
  for (unsigned i = 0; i < sizeof(keyIdxMap) / sizeof(keyIdxMap[0]); i++)
    if (keyIdxMap[i][0] == key_code) {
      setCharFromKeyPress(key_code, charsForKey[keyIdxMap[i][1]]);
      return true;
    }
  switch (key_code) {
  case KEY_KPENTER:
    dialog->SetModalResult(mrOK);
    return true;
  case KEY_ESCAPE:
    dialog->SetModalResult(mrCancel);
    return true;
  default:
    return false;
  }
  return true;
}

inline void
NumPadTextEntryWidget::SetDialog(WidgetDialog &dialog)
{
  this->dialog = &dialog;
}

class NumPadDialog : public WidgetDialog{
//  NumPadTextEntryWidget widget;
public:
  using WidgetDialog::WidgetDialog;

  void SetWidget(TCHAR *text, size_t width) {

    FinishPreliminary(std::make_unique<NumPadTextEntryWidget>(text, width) );
  }

  auto &GetWidget() noexcept {
    return static_cast<NumPadTextEntryWidget &>(WidgetDialog::GetWidget());
  }


  bool OnMouseDown(PixelPoint p) override{
      // This is the only exit for the dialog, if you don't have a keyboard attached.
      SetModalResult(mrOK);
      return true;
  }
  NumPadDialog(const TCHAR *caption, TCHAR *text, size_t width ):
    WidgetDialog(WidgetDialog::Full { },
                                                UIGlobals::GetMainWindow(),
                                                UIGlobals::GetDialogLook(),
                                                caption){};

};


void
NumPadTextEntry(TCHAR *text, size_t width, const TCHAR *caption)
{
  if (width == 0)
    width = MAX_TEXTENTRY;
  NumPadDialog dialog( caption, text, width);
//  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(text, width);
  dialog.GetWidget().SetDialog(dialog);
  if (dialog.ShowModal() == mrOK) {
    StripRight(dialog.GetWidget().GetValue());
    CopyTruncateString(text, width, dialog.GetWidget().GetValue());
  }
}
