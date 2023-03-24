// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package de.opensoar;

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
  private static final String TAG = "OpenSoar";

  private static final String NOTIFICATION_CHANNEL_ID = "opensoar";

  /**
   * Hack: this is set by onCreate(), to support the "testing"
   * package.
   */
  protected static Class<?> mainActivityClass;

  private NotificationManager notificationManager;

  @Override public void onCreate() {
    if (mainActivityClass == null)
      mainActivityClass = OpenSoar.class;

    super.onCreate();

    notificationManager = (NotificationManager)getSystemService(NOTIFICATION_SERVICE);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      String name = "OpenSoar";

      /* this disables sound: */
      int importance = NotificationManager.IMPORTANCE_LOW;

      NotificationChannel channel = new NotificationChannel(NOTIFICATION_CHANNEL_ID,
                                                            name, importance);
      notificationManager.createNotificationChannel(channel);
    }
  }

  private static Notification createNotification(Context context, PendingIntent intent) {
    Notification.Builder builder = new Notification.Builder(context)
      .setOngoing(true)
      .setContentIntent(intent)
      .setContentTitle("OpenSoar")
      .setContentText("OpenSoar is running")
      .setSmallIcon(R.drawable.notification_icon);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O)
      builder.setChannelId(NOTIFICATION_CHANNEL_ID);

    return builder.build();
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
