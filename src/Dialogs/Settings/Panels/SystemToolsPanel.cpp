// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SystemToolsPanel.hpp"
#include "Dialogs/Error.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/ProcessDialog.hpp"
#include "Interface.hpp"
#include "Language/Language.hpp"
#include "LogFile.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TwoTextRowsRenderer.hpp"
#include "SystemTools.hpp"
#include "UIGlobals.hpp"
#include "Widget/ListWidget.hpp"

#include <algorithm>
#include <cstdlib>
#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

static std::string
GetSystemToolLanguage()
{
  const char *const value = std::getenv("LANGUAGE");
  if (value == nullptr)
    return {};

  std::string_view language{value};
  const auto separator = language.find_first_of("_:.-");
  if (separator != std::string_view::npos)
    language = language.substr(0, separator);

  return std::string{language};
}

static std::vector<SystemTools::InstalledTool>
LoadSystemTools() noexcept
try
{
  return SystemTools::LoadInstalled(GetSystemToolLanguage());
}
catch (...)
{
  LogError(std::current_exception(), "Failed to select system-tool language");
  return {};
}

static bool
IsSupportedSystemTool(const SystemTools::InstalledTool &tool) noexcept
{
  return tool.descriptor.mode == SystemTools::Mode::JOB;
}

bool
HasSystemTools() noexcept
{
  const auto tools = LoadSystemTools();
  return std::any_of(tools.begin(), tools.end(), IsSupportedSystemTool);
}

static bool
ConfirmSystemTool(const SystemTools::InstalledTool &tool)
{
  const MessageBoxButton buttons[]{
    {_("Start"), IDOK},
    {_("Back"), IDCANCEL},
  };
  return ShowMessageBox(tool.descriptor.instructions.c_str(),
                        tool.descriptor.name.c_str(), buttons,
                        IDCANCEL) == IDOK;
}

static void
RunSystemTool(const SystemTools::InstalledTool &tool) noexcept
try
{
  if (CommonInterface::Calculated().flight.flying) {
    ShowMessageBox(_("Not available while flying."),
                   tool.descriptor.name.c_str(), MB_OK | MB_ICONWARNING);
    return;
  }

  if (tool.descriptor.confirmation == SystemTools::Confirmation::REQUIRED &&
      !ConfirmSystemTool(tool))
    return;

  // The confirmation may have remained open until after take-off.
  if (CommonInterface::Calculated().flight.flying) {
    ShowMessageBox(_("Not available while flying."),
                   tool.descriptor.name.c_str(), MB_OK | MB_ICONWARNING);
    return;
  }

  const char *const argv[]{tool.executable.c_str(), nullptr};
  bool completed = false;
  RunProcessDialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                   tool.descriptor.name.c_str(), argv,
                   [&completed](int) {
                     completed = true;
                     return 0;
                   },
                   tool.descriptor.description.c_str());

  if (!completed)
    ShowMessageBox(_("Cancelled"), tool.descriptor.name.c_str(), MB_OK);
}
catch (...)
{
  ShowError(std::current_exception(), tool.descriptor.name.c_str());
}

class SystemToolsPanel final : public ListWidget {
  std::vector<SystemTools::InstalledTool> tools = LoadSystemTools();
  TwoTextRowsRenderer row_renderer;

public:
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void OnPaintItem(Canvas &canvas, PixelRect rc,
                   unsigned index) noexcept override;
  bool CanActivateItem(unsigned index) const noexcept override;
  void OnActivateItem(unsigned index) noexcept override;
};

void
SystemToolsPanel::Prepare(ContainerWindow &parent,
                          const PixelRect &rc) noexcept
{
  std::erase_if(tools, [](const auto &tool) {
    return !IsSupportedSystemTool(tool);
  });

  const auto &look = UIGlobals::GetDialogLook();
  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(look.text_font, look.small_font));
  GetList().SetLength(tools.size());
}

void
SystemToolsPanel::OnPaintItem(Canvas &canvas, PixelRect rc,
                              unsigned index) noexcept
{
  if (index >= tools.size())
    return;

  row_renderer.DrawFirstRow(canvas, rc, tools[index].descriptor.name.c_str());
  row_renderer.DrawSecondRow(canvas, rc,
                             tools[index].descriptor.description.c_str());
}

bool
SystemToolsPanel::CanActivateItem(unsigned index) const noexcept
{
  return index < tools.size();
}

void
SystemToolsPanel::OnActivateItem(unsigned index) noexcept
{
  if (index < tools.size())
    RunSystemTool(tools[index]);
}

std::unique_ptr<Widget>
CreateSystemToolsPanel()
{
  return std::make_unique<SystemToolsPanel>();
}
