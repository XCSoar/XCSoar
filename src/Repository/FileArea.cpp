// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "FileArea.hpp"
#include "CountryName.hpp"
#include "util/CharUtil.hxx"

#include <algorithm>

/**
 * First filename token → ISO alpha-2.  Two-letter ISO codes are
 * handled by #GetCountryName; this table is only alpha-3 and the
 * historic map names that are not ISO.
 */
static constexpr struct {
  const char *from;
  const char *to;
} name_area[] = {
  { "ARG", "AR" },
  { "AUS", "AU" },
  { "BRA", "BR" },
  { "BUL", "BG" },
  { "CAN", "CA" },
  { "ESP", "ES" },
  { "FIN", "FI" },
  { "FRA", "FR" },
  { "GER", "DE" },
  { "HUN", "HU" },
  { "IRE", "IE" },
  { "ISR", "IL" },
  { "ITA", "IT" },
  { "MEX", "MX" },
  { "NAM", "NA" },
  { "NOR", "NO" },
  { "NZL", "NZ" },
  { "POL", "PL" },
  { "POR", "PT" },
  { "SWE", "SE" },
  { "TURKEY", "TR" },
  { "UK", "GB" },
};

std::string
NormalizeFileArea(std::string_view area) noexcept
{
  std::string out;
  out.reserve(area.size());
  for (const char ch : area) {
    if (ch == '\0')
      break;
    out.push_back(ToUpperASCII(ch));
  }
  return out;
}

std::string
GuessFileAreaFromName(std::string_view name) noexcept
{
  if (auto slash = name.find_last_of("/\\"); slash != name.npos)
    name.remove_prefix(slash + 1);
  if (auto dot = name.rfind('.'); dot != name.npos)
    name = name.substr(0, dot);

  std::string token;
  for (const char ch : name) {
    if (ch == '_' || ch == '-')
      break;
    token.push_back(ToUpperASCII(ch));
  }

  if (token.size() == 2 && GetCountryName(token) != nullptr)
    return token;

  for (const auto &i : name_area)
    if (token == i.from)
      return i.to;

  return {};
}

std::string
NormalizeFileArea(const AvailableFile &file) noexcept
{
  auto area = NormalizeFileArea(file.GetArea());
  if (!area.empty())
    return area;
  if (file.GetName() == nullptr)
    return {};
  return GuessFileAreaFromName(file.GetName());
}

bool
FileMatchesArea(const AvailableFile &file, std::string_view area) noexcept
{
  return NormalizeFileArea(file) == area;
}

std::vector<std::string>
CollectUniqueFileAreas(const std::vector<AvailableFile> &files)
{
  std::vector<std::string> areas;
  areas.reserve(files.size());
  for (const auto &file : files)
    areas.push_back(NormalizeFileArea(file));

  std::sort(areas.begin(), areas.end());
  areas.erase(std::unique(areas.begin(), areas.end()), areas.end());
  return areas;
}

unsigned
CountFilesInArea(const std::vector<AvailableFile> &files,
                 std::string_view area) noexcept
{
  unsigned count = 0;
  for (const auto &file : files)
    if (FileMatchesArea(file, area))
      ++count;
  return count;
}

void
AppendFilesInArea(const std::vector<AvailableFile> &files,
                  std::string_view area,
                  std::vector<AvailableFile> &dest)
{
  for (const auto &file : files)
    if (FileMatchesArea(file, area))
      dest.push_back(file);
}
