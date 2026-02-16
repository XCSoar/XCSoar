// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Message.hpp"
#include "PopupMessage.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"

void
Message::AddMessage(const char *text, const char *data) noexcept
{
  if (CommonInterface::main_window->popup != nullptr)
    CommonInterface::main_window->popup->AddMessage(text, data);
}
