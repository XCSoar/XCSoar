// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "InputEvents.hpp"
#include "Dialogs/Message.hpp"
#include "lua/StartFile.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "system/Path.hpp"
#include "system/FileUtil.hpp"
#include "Form/DataField/ComboList.hpp"
#include "util/StringFormat.hpp"
#include "util/StringCompare.hxx"

#include <stdexcept>

#include <windef.h> /* for MAX_PATH */

class LuaFileVisitor final : public File::Visitor {
public:
  ComboList combo_list;

  void Visit(Path path, Path filename) override {
    combo_list.Append(path.c_str(), filename.c_str());
  }
};

static AllocatedPath
SelectLuaFile(const char *path)
{
  if (StringIsEmpty(path)) {
    /* no parameter: let user select a *.lua file */
    LuaFileVisitor visitor;

    Directory::VisitSpecificFiles(LocalPath(_T("lua")), _T("*.lua"),
                                  visitor, true);
    if (visitor.combo_list.empty()) {
      ShowMessageBox(_("Not found"), _T("RunLuaFile"),
                     MB_OK|MB_ICONINFORMATION);
      return nullptr;
    }

    int i = ComboPicker(_("Select a file"), visitor.combo_list);
    if (i < 0)
      return nullptr;

    return Path(visitor.combo_list[i].string_value.c_str());
  } else if (StringEndsWith(path, _T(".lua"))) {
    /* *.lua file specified: run this file */
    return Path(path).IsAbsolute()
      ? AllocatedPath(Path(path))
      : AllocatedPath::Build(LocalPath(_T("lua")), path);
  } else {
    ShowMessageBox(_T("RunLuaFile expects *.lua parameter"),
                   _T("RunLuaFile"), MB_OK|MB_ICONINFORMATION);
    return nullptr;
  }
}

void
InputEvents::eventRunLuaFile(const char *misc)
{
  const auto path = SelectLuaFile(misc);
  if (path == nullptr)
    return;

  try {
    Lua::StartFile(path);
  } catch (...) {
    char buffer[MAX_PATH];
    StringFormat(buffer, MAX_PATH, _T("RunLuaFile %s"), misc);
    ShowError(std::current_exception(), buffer);
  }
}
