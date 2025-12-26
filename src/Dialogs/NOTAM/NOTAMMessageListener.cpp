// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "NOTAMMessageListener.hpp"

#ifdef HAVE_HTTP

#include "NOTAM/NOTAMGlue.hpp"
#include "Message.hpp"
#include "Language/Language.hpp"
#include "util/StaticString.hxx"

#include <memory>

static std::unique_ptr<NOTAMMessageListener> notam_message_listener;

NOTAMMessageListener::NOTAMMessageListener(NOTAMGlue &_glue) noexcept
  :glue(_glue)
{
  glue.AddListener(*this);
}

NOTAMMessageListener::~NOTAMMessageListener() noexcept
{
  glue.RemoveListener(*this);
}

void
NOTAMMessageListener::OnNOTAMsUpdated() noexcept
{
}

void
NOTAMMessageListener::OnNOTAMsLoadComplete(
  NOTAMLoadNotification notification) noexcept
{
  if (notification.show_fetch_failure_message)
    Message::AddMessage(_("Failed to load NOTAMs"));

  if (!notification.success || !notification.show_loaded_message)
    return;

  StaticString<100> msg;
  if (notification.loaded_count == 1)
    msg = _("Loaded 1 NOTAM");
  else
    msg.Format(_("Loaded %u NOTAMs"), notification.loaded_count);

  Message::AddMessage(msg.c_str());
}

void
InstallNOTAMMessageListener(NOTAMGlue &glue) noexcept
{
  notam_message_listener = std::make_unique<NOTAMMessageListener>(glue);
}

void
DeinitNOTAMMessageListener() noexcept
{
  notam_message_listener.reset();
}

#endif // HAVE_HTTP
