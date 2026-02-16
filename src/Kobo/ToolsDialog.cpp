// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ToolsDialog.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Widget/RowFormWidget.hpp"
#include "system/FileUtil.hpp"
#include "system/Process.hpp"
#include "util/StringAPI.hxx"

#include <vector>
#include <windef.h> /* for MAX_PATH */

struct ListItem
{
  StaticString<32> name;
  AllocatedPath path;

  ListItem(const char *_name, Path _path)
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
  : public RowFormWidget {

  const unsigned MAX_SCRIPTS = 12;

  std::vector<ListItem> list;

public:
  ToolsWidget(const DialogLook &look):RowFormWidget(look) {}

  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent,
               const PixelRect &rc) noexcept override;
};

void
ToolsWidget::Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept
{
  ScriptFileVisitor sfv(list);
  Directory::VisitFiles(Path("/mnt/onboard/XCSoarData/kobo/scripts"), sfv);

  unsigned len = list.size();
  if (len > 0)
    std::sort(list.begin(), list.end());

  unsigned max_script_buttons = std::min(len, MAX_SCRIPTS);
  for (unsigned i = 0; i < max_script_buttons; i++)
    AddButton(list[i].name, [this, i](){
      Run(list[i].path.c_str());
    });
}

void
ShowToolsDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  TWidgetDialog<ToolsWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(), look, _("Tools"));
  dialog.AddButton(_("Close"), mrOK);
  dialog.SetWidget(look);
  dialog.ShowModal();
}
