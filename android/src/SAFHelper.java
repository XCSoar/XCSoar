// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

package org.xcsoar;

import android.app.Activity;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.UriPermission;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.provider.DocumentsContract;
import android.util.Log;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Helper class that wraps Android's Storage Access Framework (SAF).
 *
 * SAF does not expose real file-system paths.  Instead, content URIs
 * ("content://…") are used to read/write documents.  The UI never
 * sees the full path — it only sees a human-readable label for each
 * volume.
 *
 * Typical flow:
 * 1.  Enumerate volumes (SD card, USB OTG) via {@link #getVolumes()}.
 * 2.  For a volume that does not yet have a persisted URI permission,
 *     launch the SAF tree-picker via {@link #buildOpenTreeIntent(StorageVolume)}.
 * 3.  In {@code onActivityResult()} forward the granted tree URI to
 *     {@link #persistTreePermission(Uri)}.
 * 4.  Use {@link #openRead(Uri, String)}, {@link #openWrite(Uri, String, boolean)}
 *     and {@link #listChildren(Uri)} through the persisted root URI.
 */
public class SAFHelper {
  private static final String TAG = "XCSoar-SAF";

  /** Request code used for {@code ACTION_OPEN_DOCUMENT_TREE} intents. */
  public static final int REQUEST_CODE_OPEN_TREE = 0x5AF0;

  private final Context context;

  public SAFHelper(Context context) {
    this.context = context;
  }

  // ---------------------------------------------------------------
  //  Volume enumeration
  // ---------------------------------------------------------------

  /**
   * Lightweight descriptor returned by {@link #getVolumes()}.
   * Java-visible only; native code accesses the fields via JNI.
   */
  public static class VolumeInfo {
    /** Unique volume UUID (or "primary" for internal). */
    public final String uuid;
    /** Human-readable description (e.g. "SD card", "USB drive"). */
    public final String description;
    /** True for removable media (SD, USB OTG). */
    public final boolean removable;
    /** If we already have a persisted tree URI for this volume, its string form; else null. */
    public final String persistedUri;

    public VolumeInfo(String uuid, String description,
                      boolean removable, String persistedUri) {
      this.uuid = uuid;
      this.description = description;
      this.removable = removable;
      this.persistedUri = persistedUri;
    }
  }

  /**
   * A directory entry with full metadata, returned by
   * {@link #listChildren}.  Accessed from native code via JNI.
   */
  public static class FileEntry {
    public final String name;
    public final boolean isDirectory;
    /** File size in bytes, or -1 if unavailable. */
    public final long size;
    /** Last-modified timestamp in ms since epoch, or -1. */
    public final long lastModified;

    public FileEntry(String name, boolean isDirectory,
                     long size, long lastModified) {
      this.name = name;
      this.isDirectory = isDirectory;
      this.size = size;
      this.lastModified = lastModified;
    }
  }

  /**
   * Enumerate storage volumes visible to the app and annotate each
   * one with the persisted SAF tree URI (if any).
   *
   * Requires API 24+.  Returns an empty array on older devices.
   */
  public VolumeInfo[] getVolumes() {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N)
      return new VolumeInfo[0];

    StorageManager sm = (StorageManager)
        context.getSystemService(Context.STORAGE_SERVICE);
    if (sm == null)
      return new VolumeInfo[0];

    List<StorageVolume> vols = sm.getStorageVolumes();
    List<UriPermission> perms =
        context.getContentResolver().getPersistedUriPermissions();

    /* Track which persisted URIs are still matched to a volume so we
       can release orphans afterwards. */
    java.util.Set<String> matchedUris = new java.util.HashSet<>();

    ArrayList<VolumeInfo> result = new ArrayList<>();
    for (StorageVolume vol : vols) {
      String uuid = vol.getUuid();
      if (uuid == null)
        uuid = "primary";

      String desc = vol.getDescription(context);
      boolean removable = vol.isRemovable();

      // Try to find a persisted tree URI that matches this volume.
      String matched = matchPersistedUri(uuid, perms);

      // Validate: check the URI is still accessible.
      if (matched != null) {
        if (isTreeUriAccessible(matched))
          matchedUris.add(matched);
        else {
          /* Volume is temporarily unavailable (e.g. unplugged).
             Keep the persisted grant so it works again when re-
             inserted; just don't advertise it as active. */
          Log.w(TAG, "Volume unavailable, keeping grant: " + matched);
          matched = null;
        }
      }

      result.add(new VolumeInfo(uuid, desc != null ? desc : uuid,
                                removable, matched));
    }

    /* Do NOT release persisted URIs that are unmatched — the volume
       may simply be unplugged or temporarily unavailable.  Grants
       are only revoked on explicit user action (see
       releaseTreePermission). */
    for (UriPermission p : perms) {
      String uriStr = p.getUri().toString();
      if (!matchedUris.contains(uriStr))
        Log.i(TAG, "Unmatched persisted grant (kept): " + uriStr);
    }

    return result.toArray(new VolumeInfo[0]);
  }

  // ---------------------------------------------------------------
  //  Intent builders — called from native, launched by Activity
  // ---------------------------------------------------------------

  /**
   * Build an {@code ACTION_OPEN_DOCUMENT_TREE} intent, optionally
   * pre-selecting the given volume (Android 10+ only).
   *
   * @param volumeUuid the UUID of the target volume, or "primary".
   * @return an Intent ready for {@code startActivityForResult()}.
   */
  public Intent buildOpenTreeIntent(String volumeUuid) {
    Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
    intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION
        | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
        | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);

    // On Android 10+ we can suggest the initial URI for the volume.
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
      StorageVolume vol = findVolumeByUuid(volumeUuid);
      if (vol != null) {
        Intent vi = vol.createOpenDocumentTreeIntent();
        if (vi != null) {
          Uri initialUri = vi.getParcelableExtra(
              DocumentsContract.EXTRA_INITIAL_URI);
          if (initialUri != null)
            intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI,
                            initialUri);
        }
      }
    }

    return intent;
  }

  // ---------------------------------------------------------------
  //  Permission persistence
  // ---------------------------------------------------------------

  /**
   * Persist the tree URI returned from the document picker so that
   * it survives reboots.  If an older permission for the same volume
   * already exists, it is released only after the new permission has
   * been successfully taken, so we never lose access.
   *
   * @return true if the permission was successfully persisted.
   */
  public boolean persistTreePermission(Uri treeUri) {
    final int flags = Intent.FLAG_GRANT_READ_URI_PERMISSION
        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION;

    // Take the new permission first — before releasing anything.
    try {
      context.getContentResolver()
          .takePersistableUriPermission(treeUri, flags);
      Log.i(TAG, "Persisted tree URI: " + treeUri);
    } catch (SecurityException e) {
      Log.e(TAG, "Failed to persist tree URI", e);
      return false;
    }

    /* Now that the new grant is safely persisted, release the old
       permission for the same volume (if any) so that only one tree
       URI per volume is persisted at any time. */
    String docId = DocumentsContract.getTreeDocumentId(treeUri);
    if (docId != null) {
      int colonIdx = docId.indexOf(':');
      String uuid = colonIdx > 0 ? docId.substring(0, colonIdx) : docId;

      List<UriPermission> perms =
          context.getContentResolver().getPersistedUriPermissions();
      String oldUri = matchPersistedUri(uuid, perms);
      if (oldUri != null && !oldUri.equals(treeUri.toString())) {
        Log.i(TAG, "Releasing old permission for volume " + uuid
                   + ": " + oldUri);
        releaseTreePermission(Uri.parse(oldUri));
      }
    }

    return true;
  }

  private void releaseTreePermission(Uri treeUri) {
    final int flags = Intent.FLAG_GRANT_READ_URI_PERMISSION
        | Intent.FLAG_GRANT_WRITE_URI_PERMISSION;
    try {
      context.getContentResolver()
          .releasePersistableUriPermission(treeUri, flags);
    } catch (SecurityException e) {
      Log.w(TAG, "releaseTreePermission failed", e);
    }
  }

  // ---------------------------------------------------------------
  //  File I/O through content URIs
  // ---------------------------------------------------------------

  /**
   * Open a file inside a SAF tree for reading.
   *
   * @param treeUri the persisted document-tree root URI
   * @param displayPath a relative display path (e.g. "XCSoarData/default.prf").
   *   Segments are resolved by walking the document tree.
   * @return an InputStream, or null on error
   */
  public InputStream openRead(String treeUri, String displayPath) {
    try {
      Uri docUri = resolveDocument(Uri.parse(treeUri), displayPath);
      if (docUri == null)
        return null;
      return context.getContentResolver().openInputStream(docUri);
    } catch (Exception e) {
      Log.e(TAG, "openRead failed: " + displayPath, e);
      return null;
    }
  }

  /**
   * Open (or create) a file inside a SAF tree for writing.
   *
   * @param treeUri the persisted document-tree root URI
   * @param displayPath relative display path
   * @param truncate if true, truncate existing content
   * @return an OutputStream, or null on error
   */
  public OutputStream openWrite(String treeUri, String displayPath,
                                boolean truncate) {
    try {
      Uri docUri = resolveOrCreateDocument(Uri.parse(treeUri), displayPath);
      if (docUri == null)
        return null;
      String mode = truncate ? "wt" : "wa";
      return context.getContentResolver().openOutputStream(docUri, mode);
    } catch (Exception e) {
      Log.e(TAG, "openWrite failed: " + displayPath, e);
      return null;
    }
  }

  /**
   * Query free and total space for the volume backing a tree URI.
   *
   * Uses StorageStatsManager (API 26+) with the volume UUID
   * extracted from the tree URI for accurate per-volume stats.
   * Falls back to StatFs for the primary volume.
   *
   * @return long[2] = { totalBytes, freeBytes }, or null on error
   */
  public long[] getSpace(String treeUri) {
    try {
      /* Extract volume UUID from the tree URI document ID.
         External storage tree URIs look like:
         content://com.android.externalstorage.documents/tree/1234-ABCD%3A
         The document ID is "1234-ABCD:" — the part before ':' is the UUID. */
      Uri parsed = Uri.parse(treeUri);
      String docId = DocumentsContract.getTreeDocumentId(parsed);
      if (docId == null)
        return null;

      String volumeId = docId.contains(":")
          ? docId.substring(0, docId.indexOf(':'))
          : docId;

      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
        android.app.usage.StorageStatsManager ssm =
            (android.app.usage.StorageStatsManager)
                context.getSystemService(Context.STORAGE_STATS_SERVICE);
        if (ssm != null) {
          java.util.UUID uuid;
          if ("primary".equalsIgnoreCase(volumeId)) {
            uuid = android.os.storage.StorageManager.UUID_DEFAULT;
          } else {
            uuid = java.util.UUID.fromString(
                volumeId.replaceFirst(
                    "(\\p{XDigit}{4})-(\\p{XDigit}{4})",
                    "00000000-0000-0000-$1-0000$2"));
          }
          long total = ssm.getTotalBytes(uuid);
          long free = ssm.getFreeBytes(uuid);
          return new long[] { total, free };
        }
      }

      /* Fallback: use StatFs on the mount point (works for primary). */
      if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
        StorageVolume vol = findVolumeByUuid(volumeId);
        if (vol != null) {
          File dir = vol.getDirectory();
          if (dir != null) {
            android.os.StatFs sf = new android.os.StatFs(dir.getPath());
            return new long[] { sf.getTotalBytes(), sf.getAvailableBytes() };
          }
        }
      }
    } catch (Exception e) {
      Log.w(TAG, "getSpace failed", e);
    }
    return null;
  }

  /**
   * List direct children of a directory with full metadata in a
   * single cursor query.
   *
   * @return array of FileEntry, or empty array
   */
  public FileEntry[] listChildren(String treeUri, String displayPath) {
    try {
      Uri parsed = Uri.parse(treeUri);
      Uri parentUri = resolveDocument(parsed, displayPath);
      if (parentUri == null)
        parentUri = DocumentsContract.buildDocumentUriUsingTree(
            parsed, DocumentsContract.getTreeDocumentId(parsed));

      Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(
          parsed, DocumentsContract.getDocumentId(parentUri));

      try (Cursor c = context.getContentResolver().query(
          childrenUri,
          new String[] {
            DocumentsContract.Document.COLUMN_DISPLAY_NAME,
            DocumentsContract.Document.COLUMN_MIME_TYPE,
            DocumentsContract.Document.COLUMN_SIZE,
            DocumentsContract.Document.COLUMN_LAST_MODIFIED
          },
          null, null, null)) {
        if (c == null)
          return new FileEntry[0];

        ArrayList<FileEntry> entries = new ArrayList<>();
        while (c.moveToNext()) {
          String name = c.getString(0);
          String mime = c.getString(1);
          boolean isDir = DocumentsContract.Document.MIME_TYPE_DIR.equals(mime);
          long size = c.isNull(2) ? -1 : c.getLong(2);
          long lastMod = c.isNull(3) ? -1 : c.getLong(3);
          entries.add(new FileEntry(name, isDir, size, lastMod));
        }
        return entries.toArray(new FileEntry[0]);
      }
    } catch (Exception e) {
      Log.e(TAG, "listChildren failed", e);
      return new FileEntry[0];
    }
  }

  /**
   * Check whether a path inside a SAF tree is a directory.
   */
  public boolean isDirectory(String treeUri, String displayPath) {
    try {
      Uri docUri = resolveDocument(Uri.parse(treeUri), displayPath);
      if (docUri == null)
        return false;

      try (Cursor c = context.getContentResolver().query(
          docUri,
          new String[] { DocumentsContract.Document.COLUMN_MIME_TYPE },
          null, null, null)) {
        if (c == null)
          return false;

        if (c.moveToFirst()) {
          String mime = c.getString(0);
          return DocumentsContract.Document.MIME_TYPE_DIR.equals(mime);
        }
        return false;
      }
    } catch (Exception e) {
      Log.e(TAG, "isDirectory failed", e);
      return false;
    }
  }

  /**
   * Delete a document (file or directory) inside a SAF tree.
   *
   * @param treeUri   the persisted tree URI for the volume
   * @param displayPath  display-name path relative to the tree root
   * @return true if the document was successfully deleted
   */
  public boolean deleteDocument(String treeUri, String displayPath) {
    try {
      Uri docUri = resolveDocument(Uri.parse(treeUri), displayPath);
      if (docUri == null)
        return false;

      return DocumentsContract.deleteDocument(
          context.getContentResolver(), docUri);
    } catch (Exception e) {
      Log.e(TAG, "deleteDocument failed", e);
      return false;
    }
  }

  // ---------------------------------------------------------------
  //  Internal helpers
  // ---------------------------------------------------------------

  /**
   * Walk the document tree segment by segment to find the document
   * identified by {@code displayPath}.
   *
   * @return the content URI for that document, or null
   */
  private Uri resolveDocument(Uri treeUri, String displayPath) {
    if (displayPath == null || displayPath.isEmpty())
      return DocumentsContract.buildDocumentUriUsingTree(
          treeUri, DocumentsContract.getTreeDocumentId(treeUri));

    String[] segments = displayPath.split("/");
    String currentDocId = DocumentsContract.getTreeDocumentId(treeUri);
    ContentResolver cr = context.getContentResolver();

    for (String seg : segments) {
      if (seg.isEmpty())
        continue;
      String childId = findChildDocId(cr, treeUri, currentDocId, seg);
      if (childId == null)
        return null;
      currentDocId = childId;
    }

    return DocumentsContract.buildDocumentUriUsingTree(treeUri, currentDocId);
  }

  /**
   * Like {@link #resolveDocument}, but creates missing path segments
   * as directories, and creates the leaf as an empty file of the
   * appropriate MIME type.
   */
  private Uri resolveOrCreateDocument(Uri treeUri, String displayPath) {
    if (displayPath == null || displayPath.isEmpty())
      return null;

    String[] segments = displayPath.split("/");
    String currentDocId = DocumentsContract.getTreeDocumentId(treeUri);
    ContentResolver cr = context.getContentResolver();

    for (int i = 0; i < segments.length; i++) {
      String seg = segments[i];
      if (seg.isEmpty())
        continue;

      String childId = findChildDocId(cr, treeUri, currentDocId, seg);
      if (childId != null) {
        currentDocId = childId;
        continue;
      }

      // Create missing segment
      boolean isLast = (i == segments.length - 1);
      Uri parentUri = DocumentsContract.buildDocumentUriUsingTree(
          treeUri, currentDocId);
      try {
        if (isLast) {
          return DocumentsContract.createDocument(
              cr, parentUri, guessMimeType(seg), seg);
        } else {
          Uri created = DocumentsContract.createDocument(
              cr, parentUri,
              DocumentsContract.Document.MIME_TYPE_DIR, seg);
          if (created == null)
            return null;
          currentDocId = DocumentsContract.getDocumentId(created);
        }
      } catch (FileNotFoundException e) {
        Log.e(TAG, "createDocument failed: " + seg, e);
        return null;
      }
    }

    return DocumentsContract.buildDocumentUriUsingTree(treeUri, currentDocId);
  }

  /**
   * Find a child document by display name inside a parent document.
   *
   * @return the child's document ID, or null if not found
   */
  private String findChildDocId(ContentResolver cr, Uri treeUri,
                                String parentDocId, String childName) {
    Uri childrenUri = DocumentsContract.buildChildDocumentsUriUsingTree(
        treeUri, parentDocId);
    try (Cursor c = cr.query(childrenUri,
        new String[] {
            DocumentsContract.Document.COLUMN_DOCUMENT_ID,
            DocumentsContract.Document.COLUMN_DISPLAY_NAME
        }, null, null, null)) {
      if (c == null)
        return null;
      while (c.moveToNext()) {
        if (childName.equals(c.getString(1)))
          return c.getString(0);
      }
    }
    return null;
  }

  /**
   * Find a StorageVolume by UUID.
   *
   * @return the matching volume, or null
   */
  private StorageVolume findVolumeByUuid(String volumeUuid) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.N)
      return null;
    StorageManager sm = (StorageManager)
        context.getSystemService(Context.STORAGE_SERVICE);
    if (sm == null)
      return null;
    for (StorageVolume vol : sm.getStorageVolumes()) {
      String vuuid = vol.getUuid();
      if (vuuid == null) vuuid = "primary";
      if (vuuid.equalsIgnoreCase(volumeUuid))
        return vol;
    }
    return null;
  }

  /**
   * Try to match a volume UUID against the list of persisted URI
   * permissions.  External-storage URIs encode the UUID in the
   * document-ID portion, e.g.
   * {@code content://…/tree/1234-ABCD%3A} (URL-encoded ":").
   * We match {@code /UUID:} or {@code /UUID%3A} to avoid substring
   * false-positives (e.g. UUID "AB" matching "ABCD").
   */
  private static String matchPersistedUri(String volumeUuid,
                                          List<UriPermission> perms) {
    final String sep1 = "/" + volumeUuid + "%3A";
    final String sep2 = "/" + volumeUuid + ":";

    for (UriPermission p : perms) {
      if (!p.isReadPermission())
        continue;
      String uriStr = p.getUri().toString();
      if (uriStr.contains(sep1) || uriStr.contains(sep2))
        return uriStr;
    }
    return null;
  }

  /**
   * Quick accessibility probe for a persisted tree URI.
   * Attempts a minimal query on the root document; returns false
   * if the URI is stale or the backing storage is gone.
   */
  private boolean isTreeUriAccessible(String treeUriStr) {
    try {
      Uri treeUri = Uri.parse(treeUriStr);
      String docId = DocumentsContract.getTreeDocumentId(treeUri);
      if (docId == null)
        return false;
      Uri docUri = DocumentsContract.buildDocumentUriUsingTree(treeUri, docId);
      try (Cursor c = context.getContentResolver().query(
          docUri,
          new String[] { DocumentsContract.Document.COLUMN_DOCUMENT_ID },
          null, null, null)) {
        return c != null && c.getCount() > 0;
      }
    } catch (Exception e) {
      return false;
    }
  }

  private static String guessMimeType(String filename) {
    if (filename == null)
      return "application/octet-stream";
    String lower = filename.toLowerCase();
    if (lower.endsWith(".igc")) return "application/x-igc";
    if (lower.endsWith(".cup")) return "text/plain";
    if (lower.endsWith(".xcm")) return "application/octet-stream";
    if (lower.endsWith(".prf")) return "text/plain";
    if (lower.endsWith(".txt")) return "text/plain";
    if (lower.endsWith(".json")) return "application/json";
    if (lower.endsWith(".xml")) return "text/xml";
    if (lower.endsWith(".dat")) return "application/octet-stream";
    if (lower.endsWith(".wpt")) return "text/plain";
    return "application/octet-stream";
  }
}
