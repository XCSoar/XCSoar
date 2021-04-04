/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "ProxyOperationEnvironment.hpp"

bool
ProxyOperationEnvironment::IsCancelled() const noexcept
{
  return other.IsCancelled();
}

void
ProxyOperationEnvironment::SetCancelHandler(std::function<void()> handler) noexcept
{
  other.SetCancelHandler(std::move(handler));
}

void
ProxyOperationEnvironment::Sleep(std::chrono::steady_clock::duration duration) noexcept
{
  other.Sleep(duration);
}

void
ProxyOperationEnvironment::SetErrorMessage(const TCHAR *text) noexcept
{
  other.SetErrorMessage(text);
}

void
ProxyOperationEnvironment::SetText(const TCHAR *text) noexcept
{
  other.SetText(text);
}

void
ProxyOperationEnvironment::SetProgressRange(unsigned range) noexcept
{
  other.SetProgressRange(range);
}

void
ProxyOperationEnvironment::SetProgressPosition(unsigned position) noexcept
{
  other.SetProgressPosition(position);
}
