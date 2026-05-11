#include "dlgFrequencyEdit.hpp"

FrequencyEditResult
dlgFrequencyEditShowModal(FrequencyListItem &freq)
{
    const DialogLook &look = UIGlobals::GetDialogLook();
    TWidgetDialog<FrequencyEditWidget>
        dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
               look, _("Frequency Editor"));
    dialog.AddButton(_("OK"), mrOK);
    dialog.AddButton(_("Cancel"), mrCancel);
    dialog.SetWidget(look, freq);
    const int result = dialog.ShowModal();

    if (result != mrOK)
        return FrequencyEditResult::CANCEL;

    if (!dialog.GetChanged())
        return FrequencyEditResult::UNMODIFIED;

    freq = dialog.GetWidget().GetValue();
    return FrequencyEditResult::MODIFIED;
}

void FrequencyEditWidget::Prepare(ContainerWindow &, const PixelRect &) noexcept
{
    AddText(_("Name"), nullptr, value.name.c_str(), this);
    AddText(_("Short Name"), nullptr, value.short_name.c_str(), this);
    AddFrequency(_("Frequency"), nullptr, value.freq, this);
}

bool FrequencyEditWidget::Save(bool &_changed) noexcept
{
    value.name = GetValueString(NAME);
    value.short_name = GetValueString(SHORTNAME);
    value.freq = GetValueRadioFrequency(FREQ);
    if(!value.freq.IsDefined())
        return false;

    _changed |= modified;
    return true;
}
