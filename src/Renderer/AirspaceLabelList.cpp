// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include <algorithm>
#include "AirspaceLabelList.hpp"
#include "Engine/Airspace/AirspaceWarningConfig.hpp"

class AirspaceLabelListCompare {
  const AirspaceWarningConfig &config;

public:
  AirspaceLabelListCompare(const AirspaceWarningConfig &_config) noexcept
    :config(_config) {}

  [[gnu::pure]]
  bool operator() (const AirspaceLabelList::Label &label1,
                   const AirspaceLabelList::Label &label2) noexcept {
    bool en1 = config.IsClassEnabled(label1.cls);
    bool en2 = config.IsClassEnabled(label2.cls);

    if(en1 == en2)
      return AirspaceAltitude::SortHighest(label2.base, label1.base);
    else if(en1)
      return false;
    else
      return true;
  }
};

void
AirspaceLabelList::Add(const GeoPoint &pos, AirspaceClass cls,
                       const AirspaceAltitude &base,
                       const AirspaceAltitude &top) noexcept
{
  if (labels.full())
    return;

  auto &label = labels.append();
  label.cls = cls;
  label.pos = pos;
  label.base = base;
  label.top = top;
}

void
AirspaceLabelList::Sort(const AirspaceWarningConfig &config) noexcept
{
  AirspaceLabelListCompare compare(config);
  std::sort(labels.begin(), labels.end(), compare);
}
