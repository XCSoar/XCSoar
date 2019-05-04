/* Copyright_License {

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

package org.xcsoar;

import java.lang.reflect.Method;
import android.app.Service;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.IBinder;
import android.os.Binder;
import android.util.Log;

/**
 * All this Service implementation does is put itself in foreground.
 * It shall reduce the risk of getting killed by the Android Activity
 * Manager, because foreground services will only be killed under
 * extreme pressure.  Since this services runs in-process, Android
 * will also not terminate our main Activity.
 *
 * XCSoar needs high reliability, because it usually runs in
 * foreground as the only active application and should not be killed
 * by an incoming phone call.
 *
 * This is bad style for general-purpose Android apps, don't imitate
 * unless you have an excuse as good as ours ;-)
 */
public class MyService extends Service {
  private static final String TAG = "XCSoar";

  private static final String NOTIFICATION_CHANNEL_ID = "xcsoar";

  /**
   * Hack: this is set by onCreate(), to support the "testing"
   * package.
   */
  protected static Class mainActivityClass;

  private NotificationManager notificationManager;

  @Override public void onCreate() {
    if (mainActivityClass == null)
      mainActivityClass = XCSoar.class;

    super.onCreate();

    notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      String name = "XCSoar";

      /* this disables sound: */
      int importance = NotificationManager.IMPORTANCE_LOW;

      NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID,
                                                            name, importance);
      notificationManager.createNotificationChannel(channel);
    }
  }

  private static Notification createNotification(Context context, PendingIntent intent) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB)
      return createNotificationOld(context, intent);

    Notification.Builder builder = new Notification.Builder(context)
      .setOngoing(true)
      .setContentIntent(intent)
      .setContentTitle("XCSoar")
      .setContentText("XCSoar is running")
      .setSmallIcon(R.drawable.notification_icon);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
      builder.setChannelId(NOTIFICATION_CHANNEL_ID);

    return builder.build();
  }

  private static Notification createNotificationOld(Context context, PendingIntent intent) {
    Notification notification = new Notification(R.drawable.notification_icon, null,
                                                 System.currentTimeMillis());

    try {
      Method method = Notification.class.getMethod("setLatestEventInfo",
                                                   Context.class,
                                                   CharSequence.class,
                                                   CharSequence.class,
                                                   PendingIntent.class);
      method.invoke(notification, context, "XCSoar", "XCSoar is running",
                    intent);
    } catch (Exception e) {
      /* ignore silently - shouldn't happen, but there's nothing we
         can do about this */
    }

    notification.flags |= Notification.FLAG_ONGOING_EVENT;
    return notification;
  }

  private void onStart() {
    /* add an icon to the notification area while XCSoar runs, to
       remind the user that we're sucking his battery empty */
    Intent intent2 = new Intent(this, mainActivityClass);
    PendingIntent contentIntent =
      PendingIntent.getActivity(this, 0, intent2, 0);
    Notification notification = createNotification(this, contentIntent);

    notificationManager.notify(1, notification);

    startForeground(1, notification);
  }

  @Override public void onStart(Intent intent, int startId) {
    /* used by API level 4 (Android 1.6) */

    onStart();
  }

  @Override public int onStartCommand(Intent intent, int flags, int startId) {
    /* used by API level 5 (Android 2.0 and newer) */

    onStart();

    /* We want this service to continue running until it is explicitly
       stopped, so return sticky */
    return START_STICKY;
  }

  @Override public void onDestroy() {
    super.onDestroy();

    notificationManager.cancel(1);
  }

  @Override public IBinder onBind(Intent intent) {
    return null;
  }
}
