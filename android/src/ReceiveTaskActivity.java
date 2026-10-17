// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.widget.TextView;
import android.util.Log;

public class ReceiveTaskActivity extends Activity {
  private static final String TAG = "XCSoar";

  @Override protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    String msg = handleIntent(getIntent());
    if (msg != null) {
      TextView tv = new TextView(this);
      tv.setText(msg);
      setContentView(tv);
    }
  }

  private String handleIntent(final Intent intent) {
    if (intent == null)
      return "No action";

    final String data = intent.getDataString();
    if (data == null)
      return "No action";

    if (!Loader.loaded)
      return "Error";

    Log.d(TAG, "Received intent data='" + data + "'");

    /* Android lower-cases the scheme of an intent URI.  The task
       text after the colon keeps its case. */
    final String msg;
    if (data.regionMatches(true, 0, "xctsk:", 0, 6))
      msg = NativeView.onReceiveXCTrackTask(data.substring(6));
    else if (data.regionMatches(true, 0, "xcs:", 0, 4) ||
             data.regionMatches(true, 0, "cup:", 0, 4))
      msg = NativeView.onReceiveTaskQRCode(data);
    else
      return "Unknown action";

    if (msg == null) {
      /* the data was handled successfully, and the main "XCSoar"
         activity shows the details - switch to it */
      startActivity(new Intent(this, XCSoar.class));
      return null;
    }

    return msg;
  }
}
