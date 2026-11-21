// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Message.hpp"
#include "PopupMessage.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"

static PopupMessage *
GetPopup() noexcept
{
  if (CommonInterface::main_window == nullptr)
    return nullptr;
  return CommonInterface::main_window->popup;
}

void
Message::AddMessage(const char *text, const char *data) noexcept
{
  if (auto *popup = GetPopup(); popup != nullptr)
    popup->AddMessage(text, data);
}

void
Message::ShowProgress(const char *text, const void *key,
                      PluggableOperationEnvironment *env) noexcept
{
  auto *popup = GetPopup();
  if (popup == nullptr)
    return;

  popup->BeginProgress(key, text);
  if (env != nullptr)
    env->SetOperationEnvironment(popup->GetProgressEnvironment(key));
}

void
Message::SetProgressText(const void *key, const char *text) noexcept
{
  if (auto *popup = GetPopup(); popup != nullptr)
    popup->SetProgressText(key, text);
}

void
Message::SetProgressRange(const void *key, unsigned range) noexcept
{
  if (auto *popup = GetPopup(); popup != nullptr)
    popup->SetProgressRange(key, range);
}

void
Message::SetProgressPosition(const void *key, unsigned position) noexcept
{
  if (auto *popup = GetPopup(); popup != nullptr)
    popup->SetProgressPosition(key, position);
}

void
Message::ClearProgress(const void *key) noexcept
{
  if (auto *popup = GetPopup(); popup != nullptr)
    popup->ClearProgress(key);
}
