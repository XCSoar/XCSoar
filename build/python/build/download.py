from typing import Sequence, Union
import http.client
import os
import socket
import ssl
from tempfile import NamedTemporaryFile
import time
import urllib.error
import urllib.request

from .verify import verify_file_digest
from .lockfile import lockfile

def __to_string_sequence(x: Union[str, Sequence[str]]) -> Sequence[str]:
    if isinstance(x, str):
        return (x,)
    else:
        return x

def __get_any(x: Union[str, Sequence[str]]) -> str:
    if isinstance(x, str):
        return x
    else:
        return x[0]

# seconds to wait before each further attempt at a URL
RETRY_DELAYS = (10, 30, 60)

# seconds a server may stay silent before the attempt counts as failed
DOWNLOAD_TIMEOUT = 60

def __is_transient(error: Exception) -> bool:
    """Might the same request succeed in a moment?  A 404, a bad
    certificate or an unknown host name will not."""

    if isinstance(error, urllib.error.HTTPError):
        return error.code == 429 or error.code >= 500

    if isinstance(error, urllib.error.URLError):
        if isinstance(error.reason, ssl.SSLCertVerificationError):
            return False
        if isinstance(error.reason, socket.gaierror):
            return error.reason.errno != socket.EAI_NONAME

    return isinstance(error, (urllib.error.URLError, http.client.HTTPException,
                              ConnectionError, TimeoutError))

def __download_one(url: str, path: str) -> None:
    print("download", url)

    # urlretrieve() has no timeout parameter
    old_timeout = socket.getdefaulttimeout()
    socket.setdefaulttimeout(DOWNLOAD_TIMEOUT)

    try:
        for delay in RETRY_DELAYS:
            try:
                urllib.request.urlretrieve(url, path)
                return
            except Exception as e:
                if not __is_transient(e):
                    raise
                print("download error:", type(e).__name__, "- retry in", delay, "s")
                time.sleep(delay)

        urllib.request.urlretrieve(url, path)
    finally:
        socket.setdefaulttimeout(old_timeout)

def __download(urls: Sequence[str], path: str) -> None:
    for url in urls[:-1]:
        try:
            __download_one(url, path)
            return
        except Exception as e:
            print("download error:", type(e).__name__)
    __download_one(urls[-1], path)

def __download_and_verify_to(urls: Sequence[str], md5: str, path: str) -> None:
    had_download_error = False
    had_digest_mismatch = False
    last_download_error: Exception = RuntimeError("download failed")

    for url in urls:
        try:
            __download_one(url, path)
        except Exception as e:
            print("download error:", type(e).__name__)
            had_download_error = True
            last_download_error = e
            continue

        try:
            if verify_file_digest(path, md5):
                return
            print("digest mismatch:", url)
            had_digest_mismatch = True
        except Exception as e:
            print("digest verification error:", type(e).__name__)
            had_digest_mismatch = True

    if had_download_error and had_digest_mismatch:
        raise RuntimeError("All download URLs failed: download errors and digest mismatches") from last_download_error
    if had_download_error:
        raise RuntimeError("All download URLs failed due to download errors") from last_download_error
    raise RuntimeError("Digest mismatch")

def download_basename(urls: Union[str, Sequence[str]]) -> str:
    return os.path.basename(__get_any(urls))

def download_and_verify(urls: Union[str, Sequence[str]], md5: str, parent_path: str) -> str:
    """Download a file, verify its MD5 checksum and return the local path."""

    base = download_basename(urls)

    os.makedirs(parent_path, exist_ok=True)
    path = os.path.join(parent_path, base)

    # protect concurrent builds by holding an exclusive lock
    with lockfile(os.path.join(parent_path, 'lock.' + base)):
        try:
            if verify_file_digest(path, md5): return path
            os.unlink(path)
        except FileNotFoundError:
            pass

        with NamedTemporaryFile(dir=parent_path) as tmp:
            __download_and_verify_to(__to_string_sequence(urls), md5, tmp.name)
            os.link(tmp.name, path)

        return path
