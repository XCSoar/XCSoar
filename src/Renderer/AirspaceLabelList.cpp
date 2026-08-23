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

    /* LabelBlock keeps the first box that claims a slot, so enabled
       classes and higher bases must come first. */
    if (en1 != en2)
      return en1;
    return AirspaceAltitude::SortHighest(label1.base, label2.base);
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
