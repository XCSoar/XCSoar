/*
  Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

package org.xcsoar;

import java.io.Closeable;
import android.os.Handler;
import android.content.Context;
import android.content.DialogInterface;
import android.app.AlertDialog;
import android.view.WindowManager;
import android.widget.EditText;
import android.text.InputType;

/*
 * A driver for voltage measurement on the IOIO board.
 */
final class TextEntryDialog
  implements Runnable, Closeable, DialogInterface.OnCancelListener
{
  /**
   * A native pointer to the C++ #AndroidTextEntryDialog instance.
   */
  final long ptr;

  AlertDialog.Builder builder;

  final EditText input;

  AlertDialog dialog;

  final Handler handler;

  TextEntryDialog(long _ptr, Context context, String title, String value, int type) {
    ptr = _ptr;

    builder = new AlertDialog.Builder(context);
    builder.setTitle(title);

    input = new EditText(context);
    input.setInputType(type);
    if (value != null)
      input.setText(value);
    builder.setView(input);

    /* the rest of the dialog setup needs to be done in Context's
       thread (XCSoar's Android UI thread) */
    handler = new Handler(context.getMainLooper());
    handler.post(this);
  }

  @Override
  public void run() {
    builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
          onResult(ptr, input.getText().toString());
          dialog.dismiss();
        }
      });

    builder.setNegativeButton("Cancel", new DialogInterface.OnClickListener() {
        @Override
        public void onClick(DialogInterface dialog, int which) {
          dialog.cancel();
        }
      });

    dialog = builder.create();
    builder = null;

    input.requestFocus();

    // show the keyboard initially
    dialog.getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);

    dialog.setOnCancelListener(this);
    dialog.show();
  }

  @Override
  public void close() {
    handler.removeCallbacks(this);
    dialog.dismiss();
  }

  @Override
  public void onCancel(DialogInterface dialog) {
    onResult(ptr, null);
  }

  native void onResult(long ptr, String value);
}
