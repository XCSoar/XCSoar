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

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/CreateWindowWidget.hpp"
#include "Widget/ArrowPagerWidget.hpp"
#include "Widget/LargeTextWidget.hpp"
#include "Look/FontDescription.hpp"
#include "Look/DialogLook.hpp"
#include "Look/Colors.hpp"
#include "ui/canvas/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "ui/canvas/Bitmap.hpp"
#include "ui/canvas/Font.hpp"
#include "Version.hpp"
#include "Inflate.hpp"
#include "util/ConvertString.hpp"
#include "util/AllocatedString.hxx"
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
  canvas.Copy({x, y}, title_size, title, {0, 0});
  y += title_size.height + Layout::FastScale(20);

  Font font;
  font.Load(FontDescription(Layout::FontScale(16)));
  canvas.Select(font);
  canvas.SetTextColor(COLOR_BLACK);
  canvas.SetBackgroundTransparent();

  canvas.DrawText({x, y}, _T("version: "));
  canvas.DrawText({x + Layout::FastScale(80), y}, XCSoar_VersionString);
  y += Layout::FastScale(22);

#ifdef GIT_COMMIT_ID
  canvas.DrawText({x, y}, _T("git: "));
  canvas.DrawText({x + Layout::FastScale(80), y}, _T(GIT_COMMIT_ID));
#endif
  y += Layout::FastScale(37);

  canvas.DrawText({x, y}, _T("more information at"));
  y += Layout::FastScale(22);

  canvas.SetTextColor(COLOR_XCSOAR);
  canvas.DrawText({x, y}, _T("http://www.xcsoar.org"));
}

static std::unique_ptr<Window>
CreateLogoPage(ContainerWindow &parent, const PixelRect &rc,
               WindowStyle style)
{
  auto window = std::make_unique<LogoPageWindow>();
  window->Create(parent, rc, style);
  return window;
}

extern "C"
{
  extern const uint8_t COPYING_gz[];
  extern const size_t COPYING_gz_size;

  extern const uint8_t NEWS_txt_gz[];
  extern const size_t NEWS_txt_gz_size;

  extern const uint8_t AUTHORS_gz[];
  extern const size_t AUTHORS_gz_size;
}

void
dlgCreditsShowModal(UI::SingleWindow &parent)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  const auto authors = InflateToString(AUTHORS_gz, AUTHORS_gz_size);
  const UTF8ToWideConverter authors2(authors.c_str());

  const auto news = InflateToString(NEWS_txt_gz, NEWS_txt_gz_size);
  const UTF8ToWideConverter news2(news.c_str());

  const auto license = InflateToString(COPYING_gz, COPYING_gz_size);
  const UTF8ToWideConverter license2(license.c_str());

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      look, _("Credits"));

  auto pager = std::make_unique<ArrowPagerWidget>(look.button,
                                                  dialog.MakeModalResultCallback(mrOK));
  pager->Add(std::make_unique<CreateWindowWidget>(CreateLogoPage));
  pager->Add(std::make_unique<LargeTextWidget>(look, authors2));
  pager->Add(std::make_unique<LargeTextWidget>(look, news2));
  pager->Add(std::make_unique<LargeTextWidget>(look, license2));

  dialog.FinishPreliminary(std::move(pager));
  dialog.ShowModal();
}
