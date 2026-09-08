// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Factory.hpp"
#include "Service.hpp"
#include "StateFile.hpp"

#ifdef HAVE_REPOSITORY_UPDATE
#include "RepositoryBackend.hpp"
#endif

std::unique_ptr<UpdateService>
CreateUpdateService(Repository::Service *repository)
{
  auto service = std::make_unique<UpdateService>(LoadUpdateState(),
                                                  SaveUpdateState);
#ifdef HAVE_REPOSITORY_UPDATE
  if (repository != nullptr)
    service->AddBackend(std::make_unique<RepositoryUpdateBackend>(*repository,
                                                                  *service));
#else
  (void)repository;
#endif
  return service;
}
