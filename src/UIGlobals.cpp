// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UIGlobals.hpp"
#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Look/Look.hpp"

UI::SingleWindow &
UIGlobals::GetMainWindow()
{
  assert(CommonInterface::main_window != nullptr);

  return *CommonInterface::main_window;
}

GlueMapWindow *
UIGlobals::GetMap()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetMap();
}

GlueMapWindow *
UIGlobals::GetMapIfActive()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetMapIfActive();
}

const DialogSettings &
UIGlobals::GetDialogSettings()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::GetUISettings().dialog;
}

const FormatSettings &
UIGlobals::GetFormatSettings()
{
  return CommonInterface::GetUISettings().format;
}

const Look &
UIGlobals::GetLook()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetLook();
}

const DialogLook &
UIGlobals::GetDialogLook()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetLook().dialog;
}

const IconLook &
UIGlobals::GetIconLook()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetLook().icon;
}

const MapLook &
UIGlobals::GetMapLook()
{
  assert(CommonInterface::main_window != nullptr);

  return CommonInterface::main_window->GetLook().map;
}
