/* Copyright_License {

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

import java.io.File;
import java.io.FileNotFoundException;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;
import android.webkit.MimeTypeMap;

/**
 * Provide access to files referred to by the Waypoint details file.
 */
public final class FileProvider extends ContentProvider {
  private static final String[] COLUMNS = {
    OpenableColumns.DISPLAY_NAME,
    OpenableColumns.SIZE,
  };

  private static final String DISPLAYNAME_FIELD = "displayName";

  @Override
  public boolean onCreate() {
    return true;
  }

  @Override
  public Cursor query(Uri uri, String[] projection, String selection,
                      String[] selectionArgs, String sortOrder) {
    final File file = getFileForUri(uri);
    if (file == null)
      throw new IllegalArgumentException("File not found");

    String displayName = uri.getQueryParameter(DISPLAYNAME_FIELD);

    if (projection == null) {
      projection = COLUMNS;
    }

    String[] cols = new String[projection.length];
    Object[] values = new Object[projection.length];
    int i = 0;
    for (String col : projection) {
      if (OpenableColumns.DISPLAY_NAME.equals(col)) {
        cols[i] = OpenableColumns.DISPLAY_NAME;
        values[i++] = (displayName == null) ? file.getName() : displayName;
      } else if (OpenableColumns.SIZE.equals(col)) {
        cols[i] = OpenableColumns.SIZE;
        values[i++] = file.length();
      }
    }

    cols = copyOf(cols, i);
    values = copyOf(values, i);

    final MatrixCursor cursor = new MatrixCursor(cols, 1);
    cursor.addRow(values);
    return cursor;
  }

  @Override
  public String getType(Uri uri) {
    File file = getFileForUri(uri);
    if (file != null) {
      String name = file.getName();
      int dot = name.lastIndexOf('.');
      if (dot >= 0) {
        String extension = name.substring(dot + 1);
        String mime = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
        if (mime != null)
          return mime;
      }
    }

    return "application/octet-stream";
  }

  @Override
  public Uri insert(Uri uri, ContentValues values) {
    throw new UnsupportedOperationException("Insert not implemented");
  }

  @Override
  public int update(Uri uri, ContentValues values, String selection,
                    String[] selectionArgs) {
    throw new UnsupportedOperationException("Update not implemented");
  }

  @Override
  public int delete(Uri uri, String selection, String[] selectionArgs) {
    return 0;
  }

  @Override
  public ParcelFileDescriptor openFile(Uri uri, String mode)
    throws FileNotFoundException {
    File file = getFileForUri(uri);
    if (file == null)
      throw new FileNotFoundException("Not found");

    if (!mode.equals("r"))
      throw new UnsupportedOperationException("Write not implemented");

    return ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
  }

  private static native String getWaypointFileForUri(int id, String filename);

  private File getFileForUri(Uri uri) {
    String path = uri.getEncodedPath();

    int i = path.indexOf('/', 1);

    String tag = Uri.decode(path.substring(1, i));
    path = path.substring(i + 1);

    if (tag.equals("waypoints")) {
      i = path.indexOf('/', 1);
      int id = Integer.parseInt(Uri.decode(path.substring(0, i)));
      String name = Uri.decode(path.substring(i + 1));
      String file = getWaypointFileForUri(id, name);
      if (file == null)
        return null;
      return new File(file);
    } else
      throw new IllegalArgumentException("Unrecognised URI tag");
  }

  private static String[] copyOf(String[] original, int newLength) {
    final String[] result = new String[newLength];
    System.arraycopy(original, 0, result, 0, newLength);
    return result;
  }

  private static Object[] copyOf(Object[] original, int newLength) {
    final Object[] result = new Object[newLength];
    System.arraycopy(original, 0, result, 0, newLength);
    return result;
  }
}
