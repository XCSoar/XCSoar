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

package org.xcsoar;

import java.io.File;
import android.app.DownloadManager;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.Uri;

class DownloadUtil extends BroadcastReceiver {
  private static DownloadUtil instance;

  static void Initialise(Context context) {
    instance = new DownloadUtil();
    context.registerReceiver(instance,
                             new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));
  }

  static void Deinitialise(Context context) {
    if (instance != null) {
      context.unregisterReceiver(instance);
      instance = null;
    }
  }

  /**
   * Check if this local URI is within the XCSoarData directory, and
   * returns the absolute path.  Returns null on mismatch.
   */
  static String matchPath(String uri) {
    /* XXX this check is a kludge to identify downloads started
       by XCSoar */
    return uri != null && uri.startsWith("file:///") &&
      uri.indexOf("/XCSoarData/") > 0
      /* strip the "file://" */
      ? uri.substring(7)
      : null;
  }

  static void enumerate(DownloadManager dm, long handler) {
    DownloadManager.Query query = new DownloadManager.Query();
    query.setFilterByStatus(DownloadManager.STATUS_PAUSED |
                            DownloadManager.STATUS_PENDING |
                            DownloadManager.STATUS_RUNNING);
    Cursor c = dm.query(query);

    if (!c.moveToFirst())
      return;

    final int columnLocalURI =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_URI);
    final int columnStatus =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_STATUS);
    final int columnSize =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_TOTAL_SIZE_BYTES);
    final int columnPosition =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR);

    do {
      final String uri = c.getString(columnLocalURI);
      final String path = matchPath(uri);
      if (path == null)
        continue;

      /* strip the "file://" */
      int status = c.getInt(columnStatus);
      long size = c.getLong(columnSize);
      long position = status == DownloadManager.STATUS_RUNNING
        ? c.getLong(columnPosition)
        : -1;

      onDownloadAdded(handler, path, size, position);
    } while (c.moveToNext());
  }

  /**
   * @param path the absolute destination path
   */
  static long enqueue(DownloadManager dm, String uri, String path) {
    DownloadManager.Request request =
      new DownloadManager.Request(Uri.parse(uri));

    /* Unfortunately, we cannot use the simpler "ExternalPublicDir"
       Android feature, because on some Samsung products, we need to
       explicitly use the "external_sd" mount instead of the built-in
       storage.  Here, we must use whatever was returned by
       FindDataPath() in LocalPath.cpp. */
    //request.setDestinationInExternalPublicDir("XCSoarData", path);
    request.setDestinationUri(Uri.fromFile(new File(path)));

    request.setAllowedOverRoaming(false);
    request.setShowRunningNotification(true);
    return dm.enqueue(request);
  }

  /**
   * Find a download with the specified name.  Returns -1 if none was
   * found.
   */
  static long findPath(DownloadManager dm, String path) {
    DownloadManager.Query query = new DownloadManager.Query();
    query.setFilterByStatus(DownloadManager.STATUS_PAUSED |
                            DownloadManager.STATUS_PENDING |
                            DownloadManager.STATUS_RUNNING);
    Cursor c = dm.query(query);

    if (!c.moveToFirst())
      return -1;

    final int columnID =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_ID);
    final int columnLocalURI =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_URI);

    do {
      final String uri = c.getString(columnLocalURI);
      if (uri != null && uri.endsWith(path))
        return c.getLong(columnID);
    } while (c.moveToNext());

    return -1;
  }

  static void cancel(DownloadManager dm, String path) {
    long id = findPath(dm, path);
    if (id >= 0) {
      dm.remove(id);
      onDownloadComplete(path, false);
    }
  }

  static native void onDownloadAdded(long handler, String path,
                                     long size, long position);
  static native void onDownloadComplete(String path, boolean success);

  static void checkComplete(DownloadManager dm) {
    DownloadManager.Query query = new DownloadManager.Query();
    query.setFilterByStatus(DownloadManager.STATUS_FAILED |
                            DownloadManager.STATUS_SUCCESSFUL);
    Cursor c = dm.query(query);

    if (c == null)
      /* should not happen, but has been observed on Android 2.3 */
      return;

    if (!c.moveToFirst())
      return;

    final int columnLocalURI =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_LOCAL_URI);
    final int columnId = c.getColumnIndexOrThrow(DownloadManager.COLUMN_ID);
    final int columnStatus =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_STATUS);

    do {
      final String uri = c.getString(columnLocalURI);
      final String path = matchPath(uri);
      if (path == null)
        continue;

      final int status = c.getInt(columnStatus);
      final boolean success = status == DownloadManager.STATUS_SUCCESSFUL;
      onDownloadComplete(path, success);

      final long id = c.getLong(columnId);
      dm.remove(id);
    } while (c.moveToNext());
  }

  @Override public void onReceive(Context context, Intent intent) {
    if (DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(intent.getAction())) {
      final DownloadManager dm = (DownloadManager)
        context.getSystemService(Context.DOWNLOAD_SERVICE);
      checkComplete(dm);
    }
  }
}
