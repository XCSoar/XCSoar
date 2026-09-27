// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PickList.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

namespace {

/**
 * One row of a stepped value list.  A page row opens another page
 * centred on #number; every other row sets #number.
 */
struct PickRow {
  bool page = false;
  double number = 0;
};

struct PickState {
  double reference = 0;
  double chosen = 0;
  bool accepted = false;
  bool page = false;
};

/**
 * How many stepped values the old list showed on either side of the
 * current one before it offered another page.
 */
static constexpr unsigned PICK_SURROUNDING = 254;

} // namespace

bool
PickList(const char *caption, double &value,
         double min_value, double max_value, double step,
         void (*format)(double value, StaticString<64> &text))
{
  if (!(step > 0) || !(max_value >= min_value) || format == nullptr)
    return false;

  double reference = std::clamp(value, min_value, max_value);

  while (true) {
    PickState state;
    state.reference = reference;

    const DialogLook &look = UIGlobals::GetDialogLook();
    auto widget = std::make_unique<GroupedListWidget>(look);
    GroupedListWidget &list = *widget;

    WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                        look, caption);

    std::vector<PickRow> rows;
    rows.reserve(PICK_SURROUNDING * 2 + 4);

    const auto apply = [&](unsigned index) {
      if (index >= rows.size())
        return;

      const PickRow &row = rows[index];
      if (row.page) {
        state.reference = row.number;
        state.page = true;
      } else {
        state.chosen = row.number;
        state.accepted = true;
      }

      dialog.SetModalResult(mrOK);
    };

    list.AddGroup(nullptr);

    const double epsilon = step / 1000.;
    const double corrected =
      std::floor((reference - min_value) / step + epsilon) * step
      + min_value;

    double first = corrected - (double)PICK_SURROUNDING * step;
    const bool more_before = first > min_value + epsilon;
    if (!more_before && first < min_value)
      first = min_value;

    const double last =
      std::min(first + (double)PICK_SURROUNDING * step * 2., max_value);

    unsigned cursor = 0;
    bool found = false;
    double last_value = first;

    const auto add = [&](bool page, double number, bool current) {
      StaticString<64> text;
      if (page)
        text = _("More");
      else
        format(number, text);

      const unsigned index = rows.size();
      rows.push_back({page, number});
      list.AddItem(text.c_str(), [&apply, index]{
        apply(index);
      });

      if (current)
        cursor = index;
    };

    if (more_before)
      add(true, first, false);

    const unsigned limit = PICK_SURROUNDING * 2 + 4;
    unsigned n = 0;
    for (double i = first;
         i <= last + epsilon && n < limit;
         i += step, ++n) {
      if (i < min_value - epsilon)
        continue;
      if (i > max_value + epsilon)
        break;

      if (!found && reference <= i + epsilon) {
        found = true;
        if (reference < i - epsilon)
          add(false, reference, true);
        add(false, i, reference >= i - epsilon);
      } else
        add(false, i, false);

      last_value = i;
    }

    if (!found)
      add(false, reference, true);

    if (last < max_value - epsilon)
      add(true, last_value, false);

    dialog.AddButton(_("Select"), [&]{
      const int i = list.GetCursorIndex();
      if (i >= 0)
        apply((unsigned)i);
    });
    dialog.AddButton(_("Cancel"), mrCancel);

    dialog.FinishPreliminary(std::move(widget));
    dialog.EnableCursorSelection();
    list.SetActionBar(dialog.GetButtonPanel());
    list.SetCursorIndex(cursor);

    if (dialog.ShowModal() != mrOK)
      return false;

    if (state.page) {
      reference = state.reference;
      continue;
    }

    if (!state.accepted)
      return false;

    value = state.chosen;
    return true;
  }
}
