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
    const bool en1 = config.IsClassEnabled(label1.cls);
    const bool en2 = config.IsClassEnabled(label2.cls);

    if (en1 != en2)
      return en1;

    if (label1.base.altitude != label2.base.altitude)
      return AirspaceAltitude::SortHighest(label1.base, label2.base);

    return label1.identity < label2.identity;
  }
};

void
AirspaceLabelList::Add(const GeoPoint &pos, AirspaceClass cls,
                       const AirspaceAltitude &base,
                       const AirspaceAltitude &top,
                       const Identity identity) noexcept
{
  Add(pos, cls, cls, base, top, identity);
}

void
AirspaceLabelList::Add(const GeoPoint &pos, AirspaceClass cls,
                       AirspaceClass border_class,
                       const AirspaceAltitude &base,
                       const AirspaceAltitude &top,
                       const Identity identity) noexcept
{
  if (labels.full())
    return;

  auto &label = labels.append();
  label.cls = cls;
  label.border_class = border_class;
  label.pos = pos;
  label.base = base;
  label.top = top;
  label.identity = identity;
}

void
AirspaceLabelList::Sort(const AirspaceWarningConfig &config) noexcept
{
  AirspaceLabelListCompare compare(config);
  std::sort(labels.begin(), labels.end(), compare);
}
