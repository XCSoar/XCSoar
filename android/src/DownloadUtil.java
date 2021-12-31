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
import java.io.File;
import android.app.DownloadManager;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.Uri;
import android.os.Environment;

/**
 * A helper class for using #DownloadManager from C++ via JNI.  It
 * provides a simpler API, only exposing the features used by XCSoar.
 */
final class DownloadUtil extends BroadcastReceiver implements Closeable {
  /**
   * A native pointer to the C++ #AndroidDownloadManager instance.
   */
  final long ptr;

  final Context context;
  final DownloadManager dm;

  final File downloadDirectory;

  DownloadUtil(long ptr, Context context) {
    this.ptr = ptr;
    this.context = context;

    dm = (DownloadManager)context.getSystemService(Context.DOWNLOAD_SERVICE);
    if (dm == null)
      /* can this really happen? */
      throw new IllegalStateException("No DownloadManager");

    context.registerReceiver(this,
                             new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));

    /* let the DownloadManager save to the app-specific directory,
       which requires no special permissions; later, we can use
       WRITE_EXTERNAL_STORAGE to move the file into XCSoarData */
    downloadDirectory = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
  }

  @Override
  public void close() {
      try {
        context.unregisterReceiver(this);
      } catch (IllegalArgumentException e) {
        /* according to Google Play crash reports, this exception gets
           thrown spuriously with an empty message, and I have no idea
           how this can happen and how to dig deeper; to avoid
           spamming the crash reports, let's just ignore it */
      }
  }

  /**
   * Check if this local URI is within the XCSoarData directory, and
   * returns the absolute path.  Returns null on mismatch.
   */
  String matchPath(Uri uri) {
    /* XXX this check is a kludge to identify downloads started
       by XCSoar */

    if (!uri.getScheme().equals("file"))
      return null;

    final String path = uri.getPath();
    final File file = new File(path);
    final File parent = file.getParentFile();
    if (parent == null || !parent.equals(downloadDirectory))
      /* not in the download directory - not for us */
      return null;

    return path;
  }

  String matchPath(String uri) {
    return uri != null
      ? matchPath(Uri.parse(uri))
      : null;
  }

  static String toFinalPath(String tmpPath) {
    final File file = new File(tmpPath);
    final String name = file.getName();
    return Uri.decode(name);
  }

  File toTmpFile(String finalPath) {
    return new File(downloadDirectory, Uri.encode(finalPath));
  }

  String toTmpPath(String finalPath) {
    return toTmpFile(finalPath).toString();
  }

  void enumerate(long handler) {
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

      onDownloadAdded(handler, toFinalPath(path), size, position);
    } while (c.moveToNext());
  }

  /**
   * @param path the absolute destination path
   */
  long enqueue(String uri, String finalPath) {
    DownloadManager.Request request =
      new DownloadManager.Request(Uri.parse(uri));

    /* Unfortunately, we cannot use the simpler "ExternalPublicDir"
       Android feature, because on some Samsung products, we need to
       explicitly use the "external_sd" mount instead of the built-in
       storage.  Here, we must use whatever was returned by
       FindDataPath() in LocalPath.cpp. */
    //request.setDestinationInExternalPublicDir("XCSoarData", path);
    request.setDestinationUri(Uri.fromFile(toTmpFile(finalPath)));

    request.setAllowedOverRoaming(false);
    request.setShowRunningNotification(true);
    return dm.enqueue(request);
  }

  /**
   * Find a download with the specified URI.  Returns -1 if none was
   * found.
   */
  long findUri(String _uri) {
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
      if (uri != null && uri.equals(_uri))
        return c.getLong(columnID);
    } while (c.moveToNext());

    return -1;
  }

  void cancel(String finalPath) {
    long id = findUri(Uri.fromFile(new File(toTmpPath(finalPath))).toString());
    if (id >= 0) {
      dm.remove(id);
      onDownloadComplete(ptr, toTmpPath(finalPath), finalPath, false);
    }
  }

  native void onDownloadAdded(long handler, String path,
                              long size, long position);
  native void onDownloadComplete(long ptr, String tmpPath, String finalPath,
                                 boolean success);

  void checkComplete() {
    DownloadManager.Query query = new DownloadManager.Query();
    query.setFilterByStatus(DownloadManager.STATUS_FAILED |
                            DownloadManager.STATUS_SUCCESSFUL);
    Cursor c;

    try {
      c = dm.query(query);
    } catch (Exception e) {
      /* should not happen, but an SQLiteException has crashed XCSoar
         on a Wiko Robby with Android 6.0 */
      return;
    }

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
      final String tmpPath = matchPath(uri);
      if (tmpPath == null)
        continue;

      final int status = c.getInt(columnStatus);
      final boolean success = status == DownloadManager.STATUS_SUCCESSFUL;
      onDownloadComplete(ptr, tmpPath, toFinalPath(tmpPath), success);

      final long id = c.getLong(columnId);
      dm.remove(id);
    } while (c.moveToNext());
  }

  @Override public void onReceive(Context context, Intent intent) {
    if (DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(intent.getAction())) {
      checkComplete();
    }
  }
}
