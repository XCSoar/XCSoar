/*
Copyright_License {
  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "MultiFilePicker.hpp"
#include "Dialogs/FilePicker.hpp"
#include "Widget/TextWidget.hpp"
#include "Widget/TwoWidgets.hpp"
#include "UIGlobals.hpp"
#include "WidgetDialog.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Look/DialogLook.hpp"
#include "ListPicker.hpp"


static constexpr int mrRemove = 601;
static constexpr int mrAdd = 600;


class MultiFilePickerSupport : public ListItemRenderer {

	TextRowRenderer row_renderer;
	std::vector<Path>& active_files;

public:

	MultiFilePickerSupport(std::vector<Path> &_active_files) :
		active_files(_active_files) {}

	unsigned CalculateLayout(const DialogLook &look) {
		return row_renderer.CalculateLayout(*look.list.font);
	}

	virtual void OnPaintItem(Canvas &canvas, const PixelRect rc,
							 unsigned i) noexcept override {

		if (active_files.empty()){
			row_renderer.DrawTextRow(canvas, rc, _(""));
			return;
		}
		row_renderer.DrawTextRow(canvas, rc, active_files[i].GetBase().c_str());
	}

};

static bool MultiFilePickerAdd(const TCHAR * caption, MultiFileDataField &df,
			 	 const TCHAR * help_text){

	if(FilePicker(caption, *df.GetFileDataField(), help_text)){
			df.SetAsInteger(df.GetFileDataField()->GetAsInteger());
		   	return true;
	}

	return false;
}


static int MultiFilePickerMain(const TCHAR *caption, MultiFileDataField &df,
					const TCHAR * help_text){

	WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
					    UIGlobals::GetDialogLook(), caption);

	std::vector<Path> active_files = df.GetPathFiles();

	MultiFilePickerSupport support(active_files);


	ListPickerWidget * file_widget = new ListPickerWidget(active_files.size(), 0,
			support.CalculateLayout(UIGlobals::GetDialogLook()),
						  support, dialog, caption, help_text);

	Widget * widget = file_widget;

	dialog.AddButton(_("Help"), [file_widget](){ file_widget->ShowHelp();} );
	dialog.AddButton(_("Add"), mrAdd );
	dialog.AddButton(_("Remove"), mrRemove);
	dialog.AddButton(_("Ok"), mrOK);
	dialog.AddButton(_("Cancel"), mrCancel);

	dialog.EnableCursorSelection();

	dialog.FinishPreliminary(widget);

	int result = dialog.ShowModal();

	if (result == mrRemove){

		unsigned int i = (int)file_widget->GetList().GetCursorIndex();
		if (!active_files.empty() && i < active_files.size())
			df.UnSet(active_files[i]);

	}

	return result;
}

bool MultiFilePicker(const TCHAR *caption, MultiFileDataField &df,
					 const TCHAR *help_text)
{
	int result;

	while ((result = MultiFilePickerMain(caption, df, help_text)) != mrOK){
		if (result == mrAdd){

			MultiFilePickerAdd(_("Add File"), df, _("Select a file to add or download a new one."));

		} else if (result == mrCancel){

			df.Restore();
			return false;
		}
	}

	return true;
}
