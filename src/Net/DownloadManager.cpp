/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "Net/DownloadManager.hpp"
#include "Android/DownloadManager.hpp"
#include "Android/Main.hpp"

static AndroidDownloadManager *download_manager;

bool
Net::DownloadManager::Initialise()
{
  assert(download_manager == NULL);

  if (!AndroidDownloadManager::Initialise(Java::GetEnv()))
    return false;

  download_manager = AndroidDownloadManager::Create(Java::GetEnv(), *context);
  return download_manager != NULL;
}

void
Net::DownloadManager::Deinitialise()
{
  delete download_manager;
  download_manager = NULL;
}

bool
Net::DownloadManager::IsAvailable()
{
  return download_manager != NULL;
}

void
Net::DownloadManager::AddListener(DownloadListener &listener)
{
  if (download_manager != NULL)
    download_manager->AddListener(listener);
}

void
Net::DownloadManager::RemoveListener(DownloadListener &listener)
{
  if (download_manager != NULL)
    download_manager->RemoveListener(listener);
}

void
Net::DownloadManager::Enqueue(const char *uri, const char *relative_path)
{
  assert(download_manager != NULL);

  download_manager->Enqueue(Java::GetEnv(), uri, relative_path);
}
