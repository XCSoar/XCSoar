// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import org.json.JSONException;
import org.json.JSONObject;
import android.content.Context;

class GliderLinkReceiver
  extends BroadcastReceiver
  implements AndroidSensor
{
  private static final String TAG = "XCSoar";
  public static final String ACTION = "link.glider.gliderlink.target_position";

  private final Context context;

  private final SensorListener listener;
  private final SafeDestruct safeDestruct = new SafeDestruct();

  private int state = STATE_LIMBO;

  public GliderLinkReceiver(Context context, SensorListener listener) {
    this.context = context;
    this.listener = listener;

    context.registerReceiver(this, new IntentFilter(ACTION));
  }

  @Override
  public void close() {
    safeDestruct.beginShutdown();

    context.unregisterReceiver(this);

    safeDestruct.finishShutdown();
  }

  @Override
  public int getState() {
    return state;
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    if (!safeDestruct.increment())
      return;

    try {
      JSONObject json = new JSONObject(intent.getStringExtra("json"));

      if (state != STATE_READY) {
        state = STATE_READY;
        listener.onSensorStateChanged();
      }

      JSONObject pos = json.getJSONObject("position");
      
      /* Sample data:
             {  
                "position":{  
                   "gid":3333,
                   "callsign":"4D",
                   "senderTime":1527263825128,
                   "receivedTime":1527263825128,
                   "latitude":37.56716816,
                   "longitude":-122.02447995000023,
                   "altitude":1701,
                   "bearing":270,
                   "gspeed":30,
                   "vspeed":3,
                   "accuracy":0
                }
             }
      */

      listener.onGliderLinkTraffic(pos.getLong("gid"),
                                   pos.getString("callsign"),
                                   pos.getDouble("latitude"),
                                   pos.getDouble("longitude"),
                                   pos.getDouble("altitude"),
                                   pos.getDouble("gspeed"),
                                   pos.getDouble("vspeed"),
                                   pos.getInt("bearing"));
    } catch (JSONException e) {
      Log.e(TAG, e.getLocalizedMessage(), e);
    } finally {
      safeDestruct.decrement();
    }
  }
}
