/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/CreateWindowWidget.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Look/FontDescription.hpp"
#include "Look/DialogLook.hpp"
#include "Look/Colors.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Font.hpp"
#include "Version.hpp"
#include "Inflate.hpp"
#include "Util/ConvertString.hpp"
#include "Resources.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

class LogoPageWindow final : public PaintWindow {
protected:
  /** from class PaintWindow */
  virtual void OnPaint(Canvas &canvas) override;
};

void
LogoPageWindow::OnPaint(Canvas &canvas)
{
  const PixelRect rc = GetClientRect();

  const unsigned width = rc.GetWidth();
  int x = rc.left + Layout::FastScale(10);
  int y = rc.top + Layout::FastScale(10);

  canvas.ClearWhite();

  Bitmap title(width > 360 ? IDB_TITLE_HD : IDB_TITLE);

  // Determine title image size
  PixelSize title_size = title.GetSize();

  // Draw 'XCSoar N.N' title
  canvas.Copy(x, y, title_size.cx, title_size.cy, title, 0, 0);
  y += title_size.cy + Layout::FastScale(20);

  Font font;
  font.Load(FontDescription(Layout::FontScale(16)));
  canvas.Select(font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();

  canvas.DrawText(x, y, _T("version: "));
  canvas.DrawText(x + Layout::FastScale(80), y, XCSoar_VersionString);
  y += Layout::FastScale(22);

  canvas.DrawText(x, y, _T("date: "));
  canvas.DrawText(x + Layout::FastScale(80), y, _T(__DATE__));
#ifdef GIT_COMMIT_ID
  y += Layout::FastScale(22);

  canvas.DrawText(x, y, _T("git: "));
  canvas.DrawText(x + Layout::FastScale(80), y, _T(GIT_COMMIT_ID));
#endif
  y += Layout::FastScale(37);

  canvas.DrawText(x, y, _T("more information at"));
  y += Layout::FastScale(22);

  canvas.SetTextColor(COLOR_XCSOAR);
  canvas.DrawText(x, y, _T("http://www.xcsoar.org"));
}

static Window *
CreateLogoPage(ContainerWindow &parent, const PixelRect &rc,
               WindowStyle style)
{
  LogoPageWindow *window = new LogoPageWindow();
  window->Create(parent, rc, style);
  return window;
}

extern "C"
{
  extern const uint8_t COPYING_gz[];
  extern const size_t COPYING_gz_size;

  extern const uint8_t AUTHORS_gz[];
  extern const size_t AUTHORS_gz_size;
}

void
dlgCreditsShowModal(SingleWindow &parent)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  char *authors = InflateToString(AUTHORS_gz, AUTHORS_gz_size);
  const UTF8ToWideConverter authors2(authors);

  char *license = InflateToString(COPYING_gz, COPYING_gz_size);
  const UTF8ToWideConverter license2(license);

  WidgetDialog dialog(look);

  ArrowPagerWidget pager(dialog, look.button);
  pager.Add(new CreateWindowWidget(CreateLogoPage));
  pager.Add(new LargeTextWidget(look, authors2));
  pager.Add(new LargeTextWidget(look, license2));

  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Credits"), &pager);
  dialog.ShowModal();
  dialog.StealWidget();

  delete[] authors;
  delete[] license;
}
