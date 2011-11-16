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

#include "Dialogs/Dialogs.h"
#include "Dialogs/Internal.hpp"
#include "Form/Tabbed.hpp"
#include "Screen/Layout.hpp"
#include "Screen/Bitmap.hpp"
#include "Screen/Font.hpp"
#include "Screen/Fonts.hpp"
#include "Screen/Key.h"
#include "ResourceLoader.hpp"
#include "Version.hpp"
#include "resource.h"

#include <assert.h>

static WndForm *wf = NULL;
static TabbedControl *tab = NULL;

static void
OnNext(gcc_unused WndButton &button)
{
  tab->NextPage();
}

static void
OnPrev(gcc_unused WndButton &button)
{
  tab->PreviousPage();
}

static bool
FormKeyDown(gcc_unused WndForm &Sender, unsigned key_code)
{
  switch (key_code) {
  case VK_LEFT:
#ifdef GNAV
  case '6':
#endif
    tab->PreviousPage();
    return true;

  case VK_RIGHT:
#ifdef GNAV
  case '7':
#endif
    tab->NextPage();
    return true;

  default:
    return false;
  }
}

static void
OnClose(gcc_unused WndButton &button)
{
  wf->SetModalResult(mrOK);
}

static void
OnLogoPaint(gcc_unused WndOwnerDrawFrame *Sender, Canvas &canvas)
{
  const UPixelScalar width = canvas.get_width();
  PixelScalar x = Layout::FastScale(10), y = x;

  canvas.clear_white();

  Bitmap title(width > 360 ? IDB_TITLE_HD : IDB_TITLE);

  // Determine title image size
  PixelSize title_size = title.get_size();

  // Draw 'XCSoar N.N' title
  canvas.copy(x, y, title_size.cx, title_size.cy, title, 0, 0);
  y += title_size.cy + Layout::FastScale(20);

  Font font;
  font.Set(Fonts::GetStandardFontFace(), Layout::FastScale(16));
  canvas.select(font);
  canvas.set_text_color(COLOR_BLACK);
  canvas.background_transparent();

  canvas.text(x, y, _T("version: "));
  canvas.text(x + Layout::FastScale(80), y, XCSoar_VersionString);
  y += Layout::FastScale(22);

  canvas.text(x, y, _T("date: "));
  canvas.text(x + Layout::FastScale(80), y, _T(__DATE__));
#ifdef GIT_COMMIT_ID
  y += Layout::FastScale(22);

  canvas.text(x, y, _T("git: "));
  canvas.text(x + Layout::FastScale(80), y, _T(GIT_COMMIT_ID));
#endif
  y += Layout::FastScale(37);

  canvas.text(x, y, _T("more information at"));
  y += Layout::FastScale(22);

  Color link(0x3E, 0x73, 0xA7);
  canvas.set_text_color(link);
  canvas.text(x, y, _T("http://www.xcsoar.org"));
}

static void
LoadTextFromResource(const TCHAR* name, const TCHAR* control)
{
  ResourceLoader::Data data = ResourceLoader::Load(name, _T("TEXT"));
  assert(data.first != NULL);
  const char *buffer = (const char *)data.first;

#ifdef _UNICODE
  int length = data.second;
  TCHAR buffer2[length + 1];
  length = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, buffer, length,
                               buffer2, length);
  buffer2[length] = _T('\0');
#else
  const char *buffer2 = buffer;
#endif

  ((WndProperty *)wf->FindByName(control))->SetText(buffer2, true);
}

static gcc_constexpr_data CallBackTableEntry CallBackTable[] = {
  DeclareCallBackEntry(OnClose),
  DeclareCallBackEntry(OnNext),
  DeclareCallBackEntry(OnPrev),
  DeclareCallBackEntry(OnLogoPaint),
  DeclareCallBackEntry(NULL)
};

void
dlgCreditsShowModal(SingleWindow &parent)
{
  wf = LoadDialog(CallBackTable, parent, Layout::landscape ?
                  _T("IDR_XML_CREDITS_L") : _T("IDR_XML_CREDITS"));
  assert(wf != NULL);

  tab = ((TabbedControl *)wf->FindByName(_T("tab")));
  assert(tab != NULL);

  wf->SetKeyDownNotify(FormKeyDown);

  LoadTextFromResource(_T("LICENSE"), _T("prpLicense"));
  LoadTextFromResource(_T("AUTHORS"), _T("prpAuthors"));

  wf->ShowModal();

  delete wf;
}
