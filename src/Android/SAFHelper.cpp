// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SAFHelper.hpp"
#include "Context.hpp"
#include "java/Class.hxx"
#include "java/Env.hxx"
#include "java/Exception.hxx"
#include "java/String.hxx"

#include <cassert>

/**
 * Parse a string into an android.net.Uri via Uri.parse().
 * Returns a local reference or nullptr on failure.
 */
static Java::LocalObject
ParseUri(JNIEnv *env, const char *uri_string) noexcept
{
  Java::Class uri_class{env, env->FindClass("android/net/Uri")};
  jmethodID parse_method = env->GetStaticMethodID(
      uri_class, "parse",
      "(Ljava/lang/String;)Landroid/net/Uri;");
  Java::String str{env, uri_string};
  return {env,
      env->CallStaticObjectMethod(uri_class, parse_method, str.Get())};
}

/**
 * Extract a Java String field from a jobject and convert to std::string.
 * Returns an empty string if the field is null.
 */
static std::string
GetStringField(JNIEnv *env, jobject obj, jfieldID field) noexcept
{
  auto s = Java::LocalRef<jstring>{
    env, (jstring)env->GetObjectField(obj, field)};
  if (!s)
    return {};
  auto chars = Java::String::GetUTFChars(env, s);
  return chars ? std::string(chars.c_str()) : std::string{};
}

static Java::TrivialClass cls;
static jmethodID ctor;
static jmethodID getVolumes_method;
static jmethodID buildOpenTreeIntent_method;
static jmethodID persistTreePermission_method;
static jmethodID openRead_method;
static jmethodID openWrite_method;
static jmethodID getSpace_method;
static jmethodID listChildren_method;
static jmethodID isDirectory_method;
static jmethodID deleteDocument_method;

// VolumeInfo inner class
static Java::TrivialClass volumeInfoCls;
static jfieldID vi_uuid_field;
static jfieldID vi_description_field;
static jfieldID vi_removable_field;
static jfieldID vi_persistedUri_field;

// FileEntry inner class
static Java::TrivialClass fileEntryCls;
static jfieldID fe_name_field;
static jfieldID fe_isDirectory_field;
static jfieldID fe_size_field;
static jfieldID fe_lastModified_field;

bool
SAFHelper::Initialise(JNIEnv *env) noexcept
{
  assert(!cls.IsDefined());
  assert(env != nullptr);

  if (!cls.FindOptional(env, "org/xcsoar/SAFHelper"))
    return false;

  ctor = env->GetMethodID(cls, "<init>",
                          "(Landroid/content/Context;)V");
  if (Java::DiscardException(env)) {
    cls.Clear(env);
    return false;
  }

  getVolumes_method = env->GetMethodID(
      cls, "getVolumes",
      "()[Lorg/xcsoar/SAFHelper$VolumeInfo;");

  buildOpenTreeIntent_method = env->GetMethodID(
      cls, "buildOpenTreeIntent",
      "(Ljava/lang/String;)Landroid/content/Intent;");

  persistTreePermission_method = env->GetMethodID(
      cls, "persistTreePermission",
      "(Landroid/net/Uri;)Z");

  openRead_method = env->GetMethodID(
      cls, "openRead",
      "(Ljava/lang/String;Ljava/lang/String;)Ljava/io/InputStream;");

  openWrite_method = env->GetMethodID(
      cls, "openWrite",
      "(Ljava/lang/String;Ljava/lang/String;Z)Ljava/io/OutputStream;");

  getSpace_method = env->GetMethodID(
      cls, "getSpace",
      "(Ljava/lang/String;)[J");

  listChildren_method = env->GetMethodID(
      cls, "listChildren",
      "(Ljava/lang/String;Ljava/lang/String;)[Lorg/xcsoar/SAFHelper$FileEntry;");

    isDirectory_method = env->GetMethodID(
      cls, "isDirectory",
      "(Ljava/lang/String;Ljava/lang/String;)Z");

  deleteDocument_method = env->GetMethodID(
      cls, "deleteDocument",
      "(Ljava/lang/String;Ljava/lang/String;)Z");

  // VolumeInfo inner class
  volumeInfoCls.Find(env, "org/xcsoar/SAFHelper$VolumeInfo");
  vi_uuid_field = env->GetFieldID(volumeInfoCls, "uuid",
                                  "Ljava/lang/String;");
  vi_description_field = env->GetFieldID(volumeInfoCls, "description",
                                         "Ljava/lang/String;");
  vi_removable_field = env->GetFieldID(volumeInfoCls, "removable", "Z");
  vi_persistedUri_field = env->GetFieldID(volumeInfoCls, "persistedUri",
                                          "Ljava/lang/String;");

  // FileEntry inner class
  fileEntryCls.Find(env, "org/xcsoar/SAFHelper$FileEntry");
  fe_name_field = env->GetFieldID(fileEntryCls, "name",
                                  "Ljava/lang/String;");
  fe_isDirectory_field = env->GetFieldID(fileEntryCls, "isDirectory", "Z");
  fe_size_field = env->GetFieldID(fileEntryCls, "size", "J");
  fe_lastModified_field = env->GetFieldID(fileEntryCls, "lastModified", "J");

  return true;
}

void
SAFHelper::Deinitialise(JNIEnv *env) noexcept
{
  fileEntryCls.ClearOptional(env);
  volumeInfoCls.ClearOptional(env);
  cls.ClearOptional(env);
}

SAFHelper::SAFHelper(JNIEnv *env, Context &context)
  :Java::GlobalObject(env,
                      Java::NewObjectRethrow(env, cls, ctor, context.Get()))
{
}

std::vector<SAFHelper::VolumeInfo>
SAFHelper::GetVolumes(JNIEnv *env) const
{
  std::vector<VolumeInfo> result;

  auto array = Java::LocalRef<jobjectArray>{
    env,
    (jobjectArray)env->CallObjectMethod(Get(), getVolumes_method)
  };

  if (!array)
    return result;

  const jsize n = env->GetArrayLength(array);
  result.reserve(n);

  for (jsize i = 0; i < n; ++i) {
    Java::LocalObject item{env, env->GetObjectArrayElement(array, i)};
    if (!item)
      continue;

    VolumeInfo vi;

    vi.uuid = GetStringField(env, item, vi_uuid_field);
    vi.description = GetStringField(env, item, vi_description_field);
    vi.removable = env->GetBooleanField(item, vi_removable_field);
    vi.persisted_uri = GetStringField(env, item, vi_persistedUri_field);

    result.push_back(std::move(vi));
  }

  return result;
}

bool
SAFHelper::PersistTreePermission(JNIEnv *env, const char *tree_uri) const
{
  auto uri = ParseUri(env, tree_uri);
  if (!uri)
    return false;

  return env->CallBooleanMethod(Get(), persistTreePermission_method,
                                uri.Get());
}

Java::LocalRef<jobject>
SAFHelper::OpenRead(JNIEnv *env,
                    const char *tree_uri,
                    const char *display_path) const
{
  Java::String treeUriStr{env, tree_uri};
  Java::String pathStr{env, display_path};
  jobject is = env->CallObjectMethod(Get(), openRead_method,
                                     treeUriStr.Get(), pathStr.Get());
  Java::RethrowException(env);

  return {env, is};
}

Java::LocalRef<jobject>
SAFHelper::OpenWrite(JNIEnv *env,
                     const char *tree_uri,
                     const char *display_path,
                     bool truncate) const
{
  Java::String treeUriStr{env, tree_uri};
  Java::String pathStr{env, display_path};
  jobject os = env->CallObjectMethod(Get(), openWrite_method,
                                     treeUriStr.Get(), pathStr.Get(),
                                     truncate ? JNI_TRUE : JNI_FALSE);
  Java::RethrowException(env);

  return {env, os};
}

std::optional<StorageDevice::Space>
SAFHelper::GetSpace(JNIEnv *env, const char *tree_uri) const
{
  Java::String uriStr{env, tree_uri};
  auto array = Java::LocalRef<jlongArray>{
    env,
    (jlongArray)env->CallObjectMethod(Get(), getSpace_method, uriStr.Get())
  };

  if (!array)
    return std::nullopt;

  jlong buf[2];
  env->GetLongArrayRegion(array, 0, 2, buf);
  return StorageDevice::Space{static_cast<uint64_t>(buf[0]),
                              static_cast<uint64_t>(buf[1])};
}

std::vector<DirEntry>
SAFHelper::ListChildren(JNIEnv *env,
                        const char *tree_uri,
                        const char *display_path) const
{
  std::vector<DirEntry> result;

  Java::String treeUriStr{env, tree_uri};
  Java::String pathStr{env, display_path};

  auto array = Java::LocalRef<jobjectArray>{
    env,
    (jobjectArray)env->CallObjectMethod(Get(), listChildren_method,
                                        treeUriStr.Get(), pathStr.Get())
  };

  Java::RethrowException(env);

  if (!array)
    return result;

  const jsize n = env->GetArrayLength(array);
  result.reserve(n);

  for (jsize i = 0; i < n; ++i) {
    Java::LocalObject item{env, env->GetObjectArrayElement(array, i)};
    if (!item)
      continue;

    DirEntry de;
    de.name = GetStringField(env, item, fe_name_field);
    de.is_directory = env->GetBooleanField(item, fe_isDirectory_field);

    jlong size_val = env->GetLongField(item, fe_size_field);
    de.size = size_val >= 0 ? std::optional<uint64_t>(size_val)
                            : std::nullopt;

    jlong mod_val = env->GetLongField(item, fe_lastModified_field);
    de.last_modified_ms = mod_val >= 0 ? std::optional<int64_t>(mod_val)
                                       : std::nullopt;

    result.push_back(std::move(de));
  }

  return result;
}

bool
SAFHelper::IsDirectory(JNIEnv *env,
                       const char *tree_uri,
                       const char *display_path) const
{
  Java::String treeUriStr{env, tree_uri};
  Java::String pathStr{env, display_path};

  jboolean r = env->CallBooleanMethod(Get(), isDirectory_method,
                                      treeUriStr.Get(), pathStr.Get());
  Java::RethrowException(env);

  return r == JNI_TRUE;
}

bool
SAFHelper::DeleteDocument(JNIEnv *env,
                          const char *tree_uri,
                          const char *display_path) const
{
  Java::String treeUriStr{env, tree_uri};
  Java::String pathStr{env, display_path};

  jboolean r = env->CallBooleanMethod(Get(), deleteDocument_method,
                                      treeUriStr.Get(), pathStr.Get());
  if (Java::DiscardException(env))
    return false;

  return r == JNI_TRUE;
}

Java::LocalRef<jobject>
SAFHelper::BuildOpenTreeIntent(JNIEnv *env,
                               const char *volume_uuid) const
{
  Java::String uuid{env, volume_uuid};
  jobject intent = env->CallObjectMethod(Get(), buildOpenTreeIntent_method,
                                         uuid.Get());
  Java::RethrowException(env);

  return {env, intent};
}
