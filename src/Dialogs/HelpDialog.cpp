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

#include "HelpDialog.hpp"
#include "Dialogs/XML.hpp"
#include "Form/Form.hpp"
#include "Screen/LargeTextWindow.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"

void
dlgHelpShowModal(SingleWindow &parent,
                 const TCHAR* Caption, const TCHAR* HelpText)
{
  if (!HelpText)
    return;

  WndForm *wf = LoadDialog(nullptr, parent,
                           Layout::landscape
                           ? _T("IDR_XML_HELP_L"): _T("IDR_XML_HELP"));

  if (wf == NULL)
    return;

  const TCHAR *prefix = _("Help");

  StaticString<100> full_caption;
  if (Caption != nullptr) {
    full_caption.Format(_T("%s: %s"), prefix, Caption);
    Caption = full_caption.c_str();
  } else
    Caption = prefix;
  wf->SetCaption(Caption);

  ((LargeTextWindow *)wf->FindByName(_T("prpHelpText")))->SetText(HelpText);

  wf->ShowModal();

  delete wf;
}
