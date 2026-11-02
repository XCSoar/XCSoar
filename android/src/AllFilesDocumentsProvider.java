/* Copyright (c) 2018-2020, Bruno de Lacheisserie
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of nor the names of its contributors may be used to
 *   endorse or promote products derived from this software without specific
 *   prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

package org.xcsoar;

import android.content.ContentResolver;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.Build;
import android.os.CancellationSignal;
import android.os.Environment;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.DocumentsContract.Document;
import android.provider.DocumentsContract.Root;
import android.provider.DocumentsProvider;
import android.system.ErrnoException;
import android.system.Os;
import android.system.StructStat;

import android.webkit.MimeTypeMap;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.util.Collections;
import java.util.LinkedList;
import java.util.Objects;

public class AllFilesDocumentsProvider extends DocumentsProvider {
  private static final String[] DEFAULT_ROOT_PROJECTION = {
    Root.COLUMN_ROOT_ID,
    Root.COLUMN_TITLE,
    Root.COLUMN_ICON,
    Root.COLUMN_FLAGS,
    Root.COLUMN_DOCUMENT_ID
  };

  private static final String[] DEFAULT_DOCUMENT_PROJECTION = {
    Document.COLUMN_DOCUMENT_ID,
    Document.COLUMN_DISPLAY_NAME,
    Document.COLUMN_MIME_TYPE,
    Document.COLUMN_SIZE,
    Document.COLUMN_LAST_MODIFIED,
    // COLUMN_ICON
    // COLUMN_SUMMARY
    Document.COLUMN_FLAGS
  };

  private static final int MAX_SEARCH_RESULTS = 20;

  private static final String ROOT_ID = "RootId";
  private static final String DOC_ID_PREFIX = "root:";

  /**
   * @param projection the requested root column projection
   * @return either the requested root column projection, or the default projection if the
   * requested projection is null.
   */
  private static String[] resolveRootProjection(String[] projection) {
    return projection != null ? projection : DEFAULT_ROOT_PROJECTION;
  }

  private static String[] resolveDocumentProjection(String[] projection) {
    return projection != null ? projection : DEFAULT_DOCUMENT_PROJECTION;
  }

  private String getAuthority() {
    return getContext().getPackageName() + ".allfiles";
  }

  private void notifyChildDocumentsChange(String parentDocumentId) {
    ContentResolver contentResolver = getContext().getContentResolver();
    contentResolver.notifyChange(
      DocumentsContract.buildChildDocumentsUri(getAuthority(), parentDocumentId),
      null, false);
  }

  @Override
  public boolean onCreate() {
    return true;
  }

  @Override
  public Cursor queryRoots(String[] projection) throws FileNotFoundException {
    MatrixCursor result = new MatrixCursor(resolveRootProjection(projection));
    MatrixCursor.RowBuilder row = result.newRow();

    Context context = Objects.requireNonNull(getContext());

    row.add(Root.COLUMN_ROOT_ID, ROOT_ID);
    row.add(Root.COLUMN_DOCUMENT_ID, DOC_ID_PREFIX);

    ApplicationInfo applicationInfo = context.getApplicationInfo();

    row.add(Root.COLUMN_ICON, applicationInfo.icon);
    row.add(Root.COLUMN_TITLE, context.getString(applicationInfo.labelRes));

    row.add(Root.COLUMN_FLAGS, Root.FLAG_LOCAL_ONLY
            | Root.FLAG_SUPPORTS_SEARCH
            | Root.FLAG_SUPPORTS_CREATE
            | Root.FLAG_SUPPORTS_IS_CHILD);

    return (result);
  }

  @Override
  public Cursor queryChildDocuments(String parentDocumentId, String[] projection, String sortOrder) throws FileNotFoundException {
    MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));
    try {
      final File root = getRootDirectory();
      File filesDir = getExistingFile(root, parentDocumentId);
      for (File file : Objects.requireNonNull(filesDir.listFiles())) {
        // Don't show hidden files/folders
        if (!file.getName().startsWith(".")) {
          addDocumentRow(result, root, file);
        }
      }
    } catch (Exception e) {
      e.printStackTrace();
      throw new FileNotFoundException();
    }
    return (result);
  }

  @Override
  public Cursor queryDocument(String documentId, String[] projection) throws FileNotFoundException {
    MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));
    try {
      final File root = getRootDirectory();
      addDocumentRow(result, root, getExistingFile(root, documentId));
    } catch (Exception e) {
      e.printStackTrace();
      throw new FileNotFoundException();
    }
    return (result);
  }

  private static String getDocumentType(final File file) throws FileNotFoundException {
    if (file.isDirectory()) {
      return DocumentsContract.Document.MIME_TYPE_DIR;
    }

    final String extension = MimeTypeMap.getFileExtensionFromUrl(Uri.fromFile(file).toString());
    final String mime = MimeTypeMap.getSingleton().getMimeTypeFromExtension(extension);
    if (mime != null) {
      return mime;
    }

    if (extension.equalsIgnoreCase("igc")) {
      return "application/vnd.fai.igc";
    }

    if (extension.equalsIgnoreCase("cup")) {
      return "application/vnd.naviter.seeyou.cup";
    }

    if (extension.equalsIgnoreCase("wpt")) {
      try {
        FileReader reader = new FileReader(file);
        BufferedReader bufferedReader = new BufferedReader(reader);
        String header = bufferedReader.readLine();
        bufferedReader.close();
        reader.close();
        if (header.contains("OziExplorer Waypoint File")) {
          return "application/vnd.oziexplorer.wpt";
        }
      } catch (IOException ignore) {
      }
    }

    if (extension.equalsIgnoreCase("gpx")) {
      return "application/gpx+xml";
    }

    return "application/octet-stream";
  }

  @Override
  public String getDocumentType(final String documentId) throws FileNotFoundException {
    return getDocumentType(getFileForDocId(documentId));
  }

  /* the primary path of FindDataPaths() in src/LocalPath.cpp */
  private File getRootDirectory() throws FileNotFoundException {
    final Context context = Objects.requireNonNull(getContext());
    try {
      for (File dir : context.getExternalFilesDirs(null))
        if (dir != null && new File(dir, "xcsoar.log").exists())
          return dir.getCanonicalFile();

      for (File dir : context.getExternalMediaDirs())
        if (dir != null)
          return dir.getCanonicalFile();

      final File legacy =
        Environment.getExternalStoragePublicDirectory("XCSoarData");
      if (legacy != null && legacy.canWrite())
        return legacy.getCanonicalFile();
    } catch (IOException e) {
      throw new FileNotFoundException(e.getMessage());
    }

    throw new FileNotFoundException("No data directory");
  }

  private static boolean isInside(File parent, File file) {
    final String parentPath = parent.getPath();
    final String path = file.getPath();
    return path.equals(parentPath) ||
      path.startsWith(parentPath + File.separator);
  }

  private static String getDocIdForFile(File root, File file) {
    final String rootPath = root.getPath();
    final String path = file.getPath();
    if (path.equals(rootPath))
      return DOC_ID_PREFIX;
    return DOC_ID_PREFIX +
      Uri.encode(path.substring(rootPath.length() + 1), "/");
  }

  private static File resolveDocId(File root, String documentId)
    throws FileNotFoundException {
    if (!documentId.startsWith(DOC_ID_PREFIX))
      throw new FileNotFoundException("Bad document id: " + documentId);

    final String relative =
      Uri.decode(documentId.substring(DOC_ID_PREFIX.length()));
    final File file;
    try {
      file = new File(root, relative).getCanonicalFile();
    } catch (IOException e) {
      throw new FileNotFoundException(e.getMessage());
    }

    if (!isInside(root, file))
      throw new FileNotFoundException("Bad document id: " + documentId);

    return file;
  }

  private static File getExistingFile(File root, String documentId)
    throws FileNotFoundException {
    final File file = resolveDocId(root, documentId);
    if (!file.exists())
      throw new FileNotFoundException("No such document: " + documentId);
    return file;
  }

  private File getFileForDocId(String documentId)
    throws FileNotFoundException {
    return getExistingFile(getRootDirectory(), documentId);
  }

  private static void checkDisplayName(String displayName)
    throws FileNotFoundException {
    if (displayName.isEmpty() || displayName.equals(".") ||
        displayName.equals("..") || displayName.indexOf('/') >= 0 ||
        displayName.indexOf('\0') >= 0)
      throw new FileNotFoundException("Bad file name: " + displayName);
  }

  private static File buildUniqueFile(File parent, String displayName,
                                      boolean isDirectory)
    throws FileNotFoundException {
    File file = new File(parent, displayName);
    if (!file.exists())
      return file;

    String name = displayName;
    String extension = "";
    final int dot = displayName.lastIndexOf('.');
    if (!isDirectory && dot > 0) {
      name = displayName.substring(0, dot);
      extension = displayName.substring(dot);
    }

    for (int i = 1; i < 32; ++i) {
      file = new File(parent, name + " (" + i + ")" + extension);
      if (!file.exists())
        return file;
    }

    throw new FileNotFoundException("No unique name for " + displayName);
  }

  private static boolean isSameFile(File a, File b) {
    try {
      final StructStat sa = Os.stat(a.getPath());
      final StructStat sb = Os.stat(b.getPath());
      return sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
    } catch (ErrnoException e) {
      return false;
    }
  }

  private void addDocumentRow(MatrixCursor result, File root, File file)
    throws FileNotFoundException {
    MatrixCursor.RowBuilder row = result.newRow();

    row.add(Document.COLUMN_DOCUMENT_ID, getDocIdForFile(root, file));
    row.add(Document.COLUMN_DISPLAY_NAME, file.getName());
    row.add(Document.COLUMN_SIZE, file.length());
    row.add(Document.COLUMN_LAST_MODIFIED, file.lastModified());
    row.add(Document.COLUMN_MIME_TYPE, getDocumentType(file));

    int flags = 0;
    if (file.canWrite()) {
      if (file.isDirectory()) {
        // allow to add file in directory
        flags |= Document.FLAG_DIR_SUPPORTS_CREATE;
      } else {
        // allow to delete and modify existing file
        flags |= Document.FLAG_SUPPORTS_DELETE;
        flags |= Document.FLAG_SUPPORTS_WRITE;
        flags |= Document.FLAG_SUPPORTS_RENAME;
      }
    }
    row.add(Document.COLUMN_FLAGS, flags);
  }

  @Override
  public boolean isChildDocument(String parentDocumentId, String documentId) {
    try {
      final File root = getRootDirectory();
      return isInside(resolveDocId(root, parentDocumentId),
                      resolveDocId(root, documentId));
    } catch (FileNotFoundException e) {
      return false;
    }
  }

  @Override
  public DocumentsContract.Path findDocumentPath(String parentDocumentId,
                                                 String childDocumentId)
    throws FileNotFoundException {
    final File root = getRootDirectory();
    final File parent = parentDocumentId != null
      ? getExistingFile(root, parentDocumentId)
      : root;
    File file = getExistingFile(root, childDocumentId);
    if (!isInside(parent, file))
      throw new FileNotFoundException(childDocumentId + " is not in " +
                                      parentDocumentId);

    final LinkedList<String> path = new LinkedList<>();
    for (;; file = file.getParentFile()) {
      path.addFirst(getDocIdForFile(root, file));
      if (file.equals(parent))
        break;
    }

    return new DocumentsContract.Path(parentDocumentId == null
                                      ? ROOT_ID
                                      : null,
                                      path);
  }

  @Override
  public ParcelFileDescriptor openDocument(String documentId, String mode, CancellationSignal signal) throws FileNotFoundException {
    return ParcelFileDescriptor.open(getFileForDocId(documentId), ParcelFileDescriptor.parseMode(mode));
  }

  @Override
  public void deleteDocument(final String documentId) throws FileNotFoundException {
    final File root = getRootDirectory();
    File file = getExistingFile(root, documentId);
    if (!file.equals(root) && file.delete()) {
      notifyChildDocumentsChange(getDocIdForFile(root, file.getParentFile()));
    } else {
      throw new FileNotFoundException("Failed to delete document id : " + documentId);
    }
  }

  @Override
  public Cursor querySearchDocuments(String rootId, String query, String[] projection) throws FileNotFoundException {
    final MatrixCursor result = new MatrixCursor(resolveDocumentProjection(projection));
    final File parent = getRootDirectory();
    final LinkedList<File> pending = new LinkedList<>();
    final String lowerQuery = query.toLowerCase();

    // Start by adding the parent to the list of files to be processed
    pending.add(parent);

    // Do while we still have unexamined files, and fewer than the max search results
    while (!pending.isEmpty() && result.getCount() < MAX_SEARCH_RESULTS) {
      // Take a file from the list of unprocessed files
      final File file = pending.removeFirst();
      if (file.isDirectory()) {
        // If it's a directory, add all its children to the unprocessed list
        Collections.addAll(pending, file.listFiles());
      } else {
        // If it's a file and it matches, add it to the result cursor.
        if (file.getName().toLowerCase().contains(lowerQuery)) {
          addDocumentRow(result, parent, file);
        }
      }
    }
    return result;
  }

  @Override
  public String createDocument(String parentDocumentId, String mimeType, String displayName) throws FileNotFoundException {
    final File root = getRootDirectory();
    final File parent = getExistingFile(root, parentDocumentId);
    if (!parent.isDirectory()) {
      throw new IllegalArgumentException("Parent document isn't a directory");
    }
    checkDisplayName(displayName);

    final boolean isDirectory = Document.MIME_TYPE_DIR.equals(mimeType);
    File file = buildUniqueFile(parent, displayName, isDirectory);

    if (isDirectory) {
      if (!file.mkdir()) {
        throw new FileNotFoundException("Failed to create directory with name " +
                                        displayName + " and documentId " + parentDocumentId);
      }
    } else {
      try {
        if (!file.createNewFile())
          throw new IOException("File exists");
        file.setWritable(true);
        file.setReadable(true);
      } catch (IOException e) {
        throw new FileNotFoundException("Failed to create document with name " +
                                        displayName + " and documentId " + parentDocumentId);
      }
    }

    notifyChildDocumentsChange(parentDocumentId);

    return getDocIdForFile(root, file);
  }

  @Override
  public String renameDocument(String documentId, String displayName) throws FileNotFoundException {
    final File root = getRootDirectory();
    File srcFile = getExistingFile(root, documentId);
    if (srcFile.equals(root))
      throw new FileNotFoundException("Cannot rename the root");
    checkDisplayName(displayName);

    File parentFile = srcFile.getParentFile();
    File dstFile = new File(parentFile, displayName);
    /* dstFile is srcFile for a case-only rename on case-insensitive storage */
    if (dstFile.exists() && !isSameFile(srcFile, dstFile))
      dstFile = buildUniqueFile(parentFile, displayName,
                                srcFile.isDirectory());

    if (!srcFile.renameTo(dstFile)) {
      throw new FileNotFoundException("Failed to rename document with name " +
                                      displayName + " and documentId " + documentId);
    }
    notifyChildDocumentsChange(getDocIdForFile(root, parentFile));
    return getDocIdForFile(root, dstFile);
  }
}
