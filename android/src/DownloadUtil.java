// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

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

    BroadcastUtil.registerReceiver(context, this,
                                    new IntentFilter(DownloadManager.ACTION_DOWNLOAD_COMPLETE));

    /* let the DownloadManager save to the app-specific directory,
       which requires no special permissions; later, we can use
       WRITE_EXTERNAL_STORAGE to move the file into XCSoarData.
       Match the same logic as LocalPath.cpp: use files directory if
       xcsoar.log exists there (backward compatibility), otherwise use media directory. */
    File dir = null;
    File[] filesDirs = context.getExternalFilesDirs(null);
    if (filesDirs != null && filesDirs.length > 0) {
      File xcsoarLog = new File(filesDirs[0], "xcsoar.log");
      if (xcsoarLog.exists()) {
        /* Old Android user - keep using files directory for backward compatibility */
        dir = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
      }
    }
    
    if (dir == null) {
      /* New installation - use media directory to match LocalPath.cpp behavior */
      File[] mediaDirs = context.getExternalMediaDirs();
      if (mediaDirs != null && mediaDirs.length > 0) {
        dir = mediaDirs[0];
      } else {
        /* Fallback to files directory if media directory is not available */
        dir = context.getExternalFilesDir(Environment.DIRECTORY_DOWNLOADS);
      }
    }
    
    downloadDirectory = dir;
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
    /* Use getAbsolutePath() for comparison to handle path normalization */
    if (parent == null) {
      return null;
    }
    String parentPath = parent.getAbsolutePath();
    String downloadPath = downloadDirectory.getAbsolutePath();
    if (!parentPath.equals(downloadPath)) {
      /* not in the download directory - not for us */
      return null;
    }

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
                            DownloadManager.STATUS_RUNNING |
                            DownloadManager.STATUS_SUCCESSFUL |
                            DownloadManager.STATUS_FAILED);
    Cursor c = dm.query(query);

    if (!c.moveToFirst())
      return;

    final int columnLocalURI = c.getColumnIndex(DownloadManager.COLUMN_LOCAL_URI);
    final int columnStatus =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_STATUS);
    final int columnSize =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_TOTAL_SIZE_BYTES);
    final int columnPosition =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_BYTES_DOWNLOADED_SO_FAR);
    final int columnId = c.getColumnIndexOrThrow(DownloadManager.COLUMN_ID);

    do {
      String path = null;
      
      /* Use COLUMN_LOCAL_URI to get the file path. For app-specific directories,
         this should always be available and be a file:// URI. */
      if (columnLocalURI >= 0) {
        final String uri = c.getString(columnLocalURI);
        if (uri != null) {
          path = matchPath(uri);
        }
      }
      
      if (path == null)
        continue;

      int status = c.getInt(columnStatus);
      
      /* Handle completed downloads as fallback (checkComplete() via broadcast
         receiver is primary, but this ensures we catch them if broadcast is missed) */
      if (status == DownloadManager.STATUS_SUCCESSFUL || status == DownloadManager.STATUS_FAILED) {
        final boolean success = status == DownloadManager.STATUS_SUCCESSFUL;
        final String finalPathName = toFinalPath(path);
        onDownloadComplete(ptr, path, finalPathName, success);
        final long id = c.getLong(columnId);
        
        /* Only remove from DownloadManager if the download failed, or if the file
           was successfully moved to a different location. If the temp and final
           paths are the same (e.g., "repository" doesn't need encoding), the move
           is a no-op and the file is still in DownloadManager's control, so we
           shouldn't remove it (which would delete the file). */
        if (!success || !path.equals(toTmpPath(finalPathName))) {
          dm.remove(id);
        }
        continue;
      }
      
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

    /* Note: setDestinationUri() with file:// URIs is deprecated on Android 10+
       for shared storage, but it still works for app-specific directories
       (like getExternalFilesDir). Since we're using the app-specific
       download directory, we can safely use setDestinationUri() here.
       This also allows us to use URI-encoded filenames for special characters. */
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
    final int columnLocalURI = c.getColumnIndex(DownloadManager.COLUMN_LOCAL_URI);

    do {
      /* Use COLUMN_LOCAL_URI to find the download. For app-specific directories,
         this should always be available and be a file:// URI. */
      if (columnLocalURI >= 0) {
        final String uri = c.getString(columnLocalURI);
        if (uri != null && uri.equals(_uri))
          return c.getLong(columnID);
      }
      
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

    final int columnLocalURI = c.getColumnIndex(DownloadManager.COLUMN_LOCAL_URI);
    final int columnId = c.getColumnIndexOrThrow(DownloadManager.COLUMN_ID);
    final int columnStatus =
      c.getColumnIndexOrThrow(DownloadManager.COLUMN_STATUS);

    do {
      String tmpPath = null;
      
      /* Use COLUMN_LOCAL_URI to get the file path. For app-specific directories,
         this should always be available and be a file:// URI. */
      if (columnLocalURI >= 0) {
        final String uri = c.getString(columnLocalURI);
        if (uri != null) {
          tmpPath = matchPath(uri);
        }
      }
      
      if (tmpPath == null)
        continue;

      final int status = c.getInt(columnStatus);
      final boolean success = status == DownloadManager.STATUS_SUCCESSFUL;
      final String finalPathName = toFinalPath(tmpPath);
      onDownloadComplete(ptr, tmpPath, finalPathName, success);

      final long id = c.getLong(columnId);
      /* Only remove from DownloadManager if the download failed, or if the file
         was successfully moved to a different location. If the temp and final
         paths are the same (e.g., "repository" doesn't need encoding), the move
         is a no-op and the file is still in DownloadManager's control, so we
         shouldn't remove it (which would delete the file). */
      if (!success || !tmpPath.equals(toTmpPath(finalPathName))) {
        dm.remove(id);
      }
    } while (c.moveToNext());
  }

  @Override public void onReceive(Context context, Intent intent) {
    if (DownloadManager.ACTION_DOWNLOAD_COMPLETE.equals(intent.getAction())) {
      checkComplete();
    }
  }
}
