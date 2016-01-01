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

#include "ToolsDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "OS/FileUtil.hpp"
#include "OS/Process.hpp"
#include "Util/StringAPI.hxx"

#include <vector>
#include <windef.h> /* for MAX_PATH */

struct ListItem
{
  StaticString<32> name;
  AllocatedPath path;

  ListItem(const TCHAR *_name, Path _path)
    :name(_name), path(_path) {}

  bool operator<(const ListItem &i2) const {
    return StringCollate(name, i2.name) < 0;
  }
};


class ScriptFileVisitor: public File::Visitor
{
  std::vector<ListItem> &list;

public:
  ScriptFileVisitor(std::vector<ListItem> &_list):list(_list) {}

  void Visit(Path path, Path filename) override {
    list.emplace_back(filename.c_str(), path);
  }
};


class ToolsWidget final
  : public RowFormWidget, ActionListener {

  const unsigned MAX_SCRIPTS = 12;

  std::vector<ListItem> list;

public:
  ToolsWidget(const DialogLook &look):RowFormWidget(look) {}

  /* virtual methods from class Widget */
  virtual void Prepare(ContainerWindow &parent,
                       const PixelRect &rc) override;

private:
  /* virtual methods from class ActionListener */
  virtual void OnAction(int id) override;

};


void
ToolsWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  ScriptFileVisitor sfv(list);
  Directory::VisitFiles(Path(_T("/mnt/onboard/XCSoarData/kobo/scripts")), sfv);

  unsigned len = list.size();
  if (len > 0)
    std::sort(list.begin(), list.end());

  unsigned max_script_buttons = std::min(len, MAX_SCRIPTS);
  for (unsigned i = 0; i < max_script_buttons; i++)
    AddButton(list[i].name, *this, i);
}

void
ToolsWidget::OnAction(int id)
{
  if (id >= 0 && id < (int) list.size())
    Run(list[id].path.c_str());
}

void
ShowToolsDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  ToolsWidget widget(look);
  WidgetDialog dialog(look);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Tools"), &widget);
  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
  dialog.StealWidget();
}
