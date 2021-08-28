from build.verify import verify_file_digest
from .lockfile import lockfile
import os
from tempfile import NamedTemporaryFile
import urllib.request
import sys

def __download(url, alternative_url, path):
    print("download", url)
    try:
        urllib.request.urlretrieve(url, path)
    except:
        if alternative_url is None:
          raise
        print("download error:", sys.exc_info()[0])
        print("download (alternative location)", alternative_url)
        urllib.request.urlretrieve(alternative_url, path)

def __download_and_verify_to(url, alternative_url, md5, path):
    __download(url, alternative_url, path)
    if not verify_file_digest(path, md5):
        raise RuntimeError("Digest mismatch")

def download_and_verify(url, alternative_url, md5, parent_path):
    """Download a file, verify its MD5 checksum and return the local path."""

    os.makedirs(parent_path, exist_ok=True)
    path = os.path.join(parent_path, os.path.basename(url))

    # protect concurrent builds by holding an exclusive lock
    with lockfile(os.path.join(parent_path, 'lock.' + os.path.basename(url))):
        try:
            if verify_file_digest(path, md5): return path
            os.unlink(path)
        except FileNotFoundError:
            pass

        with NamedTemporaryFile(dir=parent_path) as tmp:
            __download_and_verify_to(url, alternative_url, md5, tmp.name)
            os.link(tmp.name, path)

        return path
