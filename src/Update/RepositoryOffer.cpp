// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "RepositoryOffer.hpp"

#include "Repository/FileRepository.hpp"
#include "util/UriUtil.hpp"
#include "util/TruncateString.hpp"
#include "util/UTF8.hpp"
#include "util/VersionNumber.hxx"

#include <array>
#include <cstring>

namespace UpdateRepository {

static bool
CopyRemoteText(auto &destination, std::string_view value) noexcept
{
  if (!ValidateUTF8(value))
    return false;

  std::array<char, 385> source;
  if (value.size() >= source.size())
    return false;

  std::memcpy(source.data(), value.data(), value.size());
  source[value.size()] = '\0';
  CopyTruncateString(destination.buffer(), destination.capacity(), source.data());
  return true;
}

std::optional<UpdateCheckResult>
FindOffer(const FileRepository &repository, std::string_view target,
          std::string_view allowed_hosts,
          std::string_view current_version) noexcept
{
  const auto installed_version = VersionNumber::Parse(current_version);
  if (!installed_version)
    return std::nullopt;

  std::optional<VersionNumber> best_version;
  std::optional<UpdateInfo> best_info;
  bool found_compatible = false;

  for (const auto &file : repository) {
    if (file.type != FileType::SOFTWARE_UPDATE || !file.software_update)
      continue;

    const auto &metadata = *file.software_update;
    if (metadata.target != target || metadata.channel != "stable")
      continue;

    const auto offered_version = VersionNumber::Parse(metadata.version);
    const std::string_view offer_id = metadata.offer_id.empty()
      ? std::string_view{metadata.version}
      : std::string_view{metadata.offer_id};
    if (!offered_version || !IsValidUpdateOfferId(offer_id) ||
        !IsAllowedUrl(file.uri, allowed_hosts))
      continue;

    if (*offered_version <= *installed_version) {
      found_compatible = true;
      continue;
    }
    if (best_version && *offered_version <= *best_version)
      continue;

    UpdateInfo info;
    if (metadata.version.size() >= info.version.capacity() ||
        file.uri.size() >= info.handoff_url.capacity())
      continue;

    info.backend_id = UpdateBackendId::REPOSITORY;
    info.offer_id.SetASCII(offer_id);
    if (!CopyRemoteText(info.version, metadata.version) ||
        !CopyRemoteText(info.summary, file.description) ||
        !CopyRemoteText(info.source,
                        metadata.source.empty() ? "XCSoar" : metadata.source) ||
        !CopyRemoteText(info.handoff_url, file.uri))
      continue;

    found_compatible = true;
    best_version = offered_version;
    best_info = std::move(info);
  }

  if (best_info) {
    UpdateCheckResult result;
    result.state = UpdateState::AVAILABLE;
    result.info = std::move(best_info);
    return result;
  }

  if (found_compatible) {
    UpdateCheckResult result;
    result.state = UpdateState::UP_TO_DATE;
    return result;
  }

  return std::nullopt;
}

} // namespace UpdateRepository
