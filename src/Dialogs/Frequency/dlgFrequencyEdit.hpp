// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Form/DataField/Enum.hpp"
#include "Form/DataField/Listener.hpp"
#include "UIGlobals.hpp"
#include "Waypoint/Waypoint.hpp"
#include "FormatSettings.hpp"
#include "Language/Language.hpp"
#include "dlgUserFrequencyList.hpp"

enum FrequencyEditResult
{
    CANCEL,
    UNMODIFIED,
    MODIFIED
};

class FrequencyEditWidget final : public RowFormWidget, DataFieldListener
{
    enum Rows
    {
        NAME,
        SHORTNAME,
        FREQ
    };

    FrequencyListItem value;

    bool modified;

public:
    FrequencyEditWidget(const DialogLook &look, const FrequencyListItem &_value) noexcept
        : RowFormWidget(look), value(_value), modified(false) {}

    const FrequencyListItem &GetValue() const
    {
        return value;
    }

private:
    /* virtual methods from Widget */
    void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
    bool Save(bool &changed) noexcept override;

    /* virtual methods from DataFieldListener */
    void OnModified(DataField &) noexcept override
    {
        modified = true;
    }
};

FrequencyEditResult
dlgFrequencyEditShowModal(FrequencyListItem &way_point);
