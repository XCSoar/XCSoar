/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "InputEvents.hpp"
#include "Dialogs/Message.hpp"

#ifdef USE_LUA
#include "Lua/StartFile.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "OS/PathName.hpp"
#include "OS/FileUtil.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Util/Error.hxx"
#include "Util/StringFormat.hpp"
#include "Util/StringUtil.hpp"
#include "Util/StringAPI.hxx"

extern "C" {
#include <lua.h>
}

#include <windef.h> // for MAX_PATH

class LuaFileVisitor final : public File::Visitor {
public:
  ComboList combo_list;

  void Visit(const TCHAR *path, const TCHAR *filename) override {
    combo_list.Append(combo_list.size(), path, filename);
  }
};

static const TCHAR *
SelectLuaFile(TCHAR *buffer, const TCHAR *path)
{
  if (StringIsEmpty(path)) {
    /* no parameter: let user select a *.lua file */
    LuaFileVisitor visitor;

    Directory::VisitSpecificFiles(LocalPath(buffer, _T("lua")), _T("*.lua"),
                                  visitor, true);
    if (visitor.combo_list.empty()) {
      ShowMessageBox(_("Not found"), _T("RunLuaFile"),
                     MB_OK|MB_ICONINFORMATION);
      return nullptr;
    }

    int i = ComboPicker(_("Select a file"), visitor.combo_list);
    if (i < 0)
      return nullptr;

    UnsafeCopyString(buffer, visitor.combo_list[i].string_value);
    return buffer;
  } else if (StringEndsWith(path, _T(".lua"))) {
    /* *.lua file specified: run this file */
    return IsAbsolutePath(path)
      ? path
      : LocalPath(buffer, _T("lua"), path);
  } else {
    ShowMessageBox(_T("RunLuaFile expects *.lua parameter"),
                   _T("RunLuaFile"), MB_OK|MB_ICONINFORMATION);
    return nullptr;
  }
}

#endif

void
InputEvents::eventRunLuaFile(const TCHAR *misc)
{
#ifdef USE_LUA
  TCHAR buffer[MAX_PATH];
  const TCHAR *path = SelectLuaFile(buffer, misc);
  if (path == nullptr)
    return;

  Error error;
  if (!Lua::StartFile(path, error)) {
    StringFormat(buffer, MAX_PATH, _T("RunLuaFile %s"), misc);
    ShowError(error, buffer);
  }
#else
  ShowMessageBox(_T("Lua is disabled."), _T("RunLuaFile"),
                 MB_OK|MB_ICONWARNING);
#endif
}
