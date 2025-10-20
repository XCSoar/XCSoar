#!/usr/bin/env python3
"""Verify availability and checksums of source URLs declared in libs.py.

This module validates that all source URLs defined in the XCSoar build system
are accessible and optionally verifies their checksums. By default, it only
reports broken URLs to reduce noise.

Usage:
    python3 -m verify_sources [OPTIONS]

Options:
    --checksum      Verify checksums by downloading full archives
    --timeout N     Per-URL timeout in seconds (default: 15)
    --json          Output machine-readable JSON format
    --verbose       Show all URLs, not just broken ones

Exit Codes:
    0   All checks passed
    1   One or more failures (unavailable URLs or checksum mismatches)

The script automatically detects hash algorithms by digest length and uses
efficient streaming for checksum verification to avoid memory issues with
large archives. For HTTP(S) URLs, it uses HEAD requests first, falling back
to partial GET requests if needed. FTP URLs are checked using SIZE or NLST.
"""

import argparse
import ftplib
import hashlib
import json
import os
import sys
import time
import traceback
from dataclasses import dataclass
from typing import List, Optional, Tuple
from urllib.error import HTTPError, URLError
from urllib.parse import urlparse
from urllib import request as urlreq

# Set up path to find the build package (following XCSoar pattern)
script_dir = os.path.dirname(os.path.abspath(__file__))
build_python_dir = os.path.dirname(script_dir)  # build/python
sys.path[0] = build_python_dir

from build import libs
from build.verify import guess_digest_algorithm, feed_file

# Constants
DEFAULT_TIMEOUT = 15.0
HTTP_FORBIDDEN = 403
HTTP_METHOD_NOT_ALLOWED = 405
CHUNK_SIZE = 65536

def iter_projects():
    """Yield project definitions from the libs module.
    
    Returns:
        Iterator of (name, project_obj) tuples for projects with 'url' and 'md5' attributes.
    """
    for name, project in vars(libs).items():
        if not name.startswith('_') and hasattr(project, 'url') and hasattr(project, 'md5'):
            yield name, project


def normalize_urls(url_field) -> List[str]:
    """Normalize URL field to a list of strings.
    
    Args:
        url_field: String URL or sequence of URLs
        
    Returns:
        List of URL strings, empty if input is invalid
    """
    if isinstance(url_field, str):
        return [url_field]
    try:
        return [u for u in url_field if isinstance(u, str)]
    except TypeError:
        return []


def detect_algo(digest: str) -> Optional[str]:
    """Detect hash algorithm name from digest string.
    
    Args:
        digest: Hexadecimal digest string
        
    Returns:
        Algorithm name (e.g., 'md5', 'sha256') or None if unknown
    """
    algo_func = guess_digest_algorithm(digest)
    if algo_func is None:
        return None
    return algo_func().name

@dataclass
class UrlCheckResult:
    """Result of URL availability and checksum verification.
    
    Attributes:
        project: Project name
        url: URL that was checked
        ok: True if URL is accessible
        status: HTTP status code or protocol indicator
        elapsed: Time taken for the check in seconds
        error: Error message if check failed
        checksum_ok: True/False/None for checksum verification result
        checksum_error: Checksum error message if applicable
    """
    project: str
    url: str
    ok: bool
    status: str
    elapsed: float
    error: Optional[str] = None
    checksum_ok: Optional[bool] = None
    checksum_error: Optional[str] = None


def _check_http_url(url: str, timeout: float) -> Tuple[bool, str, Optional[str]]:
    """Check HTTP/HTTPS URL availability using HEAD request with GET fallback.
    
    Args:
        url: HTTP(S) URL to check
        timeout: Request timeout in seconds
        
    Returns:
        Tuple of (is_accessible, status_code, error_message)
    """
    req = urlreq.Request(url, method='HEAD')
    try:
        with urlreq.urlopen(req, timeout=timeout) as resp:
            return True, f"{resp.status}", None
    except HTTPError as he:
        if he.code in (HTTP_FORBIDDEN, HTTP_METHOD_NOT_ALLOWED):
            try:
                req_get = urlreq.Request(url, headers={'Range': 'bytes=0-0'})
                with urlreq.urlopen(req_get, timeout=timeout) as resp:
                    code = getattr(resp, 'status', 200)
                    return True, f"{code}", None
            except Exception as e2:
                return False, f"{he.code}", f"HEAD->GET fail: {e2}" if str(e2) else str(he)
        return False, f"{he.code}", str(he)
    except URLError as ue:
        return False, 'URLERR', str(ue)
    except Exception as e:
        return False, 'EXC', str(e)


def _check_ftp_url(parsed_url, timeout: float) -> Tuple[bool, str, Optional[str]]:
    """Check FTP URL availability using SIZE command with NLST fallback.
    
    Args:
        parsed_url: Parsed URL object with FTP scheme
        timeout: Connection timeout in seconds
        
    Returns:
        Tuple of (is_accessible, 'FTP', error_message)
    """
    try:
        if not parsed_url.hostname:
            return False, 'FTP', 'No hostname'
        ftp = ftplib.FTP(parsed_url.hostname, timeout=timeout)
        ftp.login()
        path = parsed_url.path
        try:
            ftp.size(path)
            ftp.quit()
            return True, 'FTP', None
        except Exception:
            try:
                dirn, _, fname = path.rpartition('/')
                ftp.cwd(dirn or '/')
                listing = ftp.nlst()
                ftp.quit()
                if fname in listing:
                    return True, 'FTP', None
                return False, 'FTP', 'Not found in listing'
            except Exception as e2:
                return False, 'FTP', f'FTP error: {e2}'
    except Exception as e:
        return False, 'FTP', str(e)


def head_or_probe(url: str, timeout: float) -> Tuple[bool, str, Optional[str]]:
    """Check URL availability using protocol-appropriate methods.
    
    Args:
        url: URL to check (HTTP, HTTPS, or FTP)
        timeout: Request timeout in seconds
        
    Returns:
        Tuple of (is_accessible, status_indicator, error_message)
    """
    parsed = urlparse(url)
    
    if parsed.scheme in ('http', 'https'):
        return _check_http_url(url, timeout)
    elif parsed.scheme == 'ftp':
        return _check_ftp_url(parsed, timeout)
    else:
        return False, 'SCHEME', f"Unsupported scheme: {parsed.scheme}"


def verify_checksum(url: str, digest: str, algo: str, timeout: float) -> Tuple[bool, Optional[str]]:
    """Verify checksum by downloading and hashing the file.
    
    Args:
        url: URL to download and verify
        digest: Expected hexadecimal digest
        algo: Hash algorithm name (e.g., 'md5', 'sha256')
        timeout: Download timeout in seconds
        
    Returns:
        Tuple of (checksum_matches, error_message)
    """
    h = hashlib.new(algo)
    try:
        req = urlreq.Request(url)
        with urlreq.urlopen(req, timeout=timeout) as resp:
            feed_file(h, resp)
        comp = h.hexdigest().lower()
        if comp != digest.lower():
            return False, f"mismatch: expected {digest} got {comp}"
        return True, None
    except Exception as e:
        return False, str(e)


def _check_project_urls(name: str, proj, check_checksums: bool, timeout: float) -> List[UrlCheckResult]:
    """Check all URLs for a single project."""
    results = []
    urls = normalize_urls(getattr(proj, 'url', []))
    digest = getattr(proj, 'md5', None)
    algo = detect_algo(digest) if digest else None
    proj_label = getattr(proj, 'name', name)

    for url in urls:
        start = time.time()
        ok, status, err = head_or_probe(url, timeout)
        elapsed = time.time() - start
        
        checksum_ok = None
        checksum_err = None
        if ok and check_checksums and algo and digest:
            checksum_ok, checksum_err = verify_checksum(url, digest, algo, timeout)
            
        results.append(UrlCheckResult(proj_label, url, ok, status, elapsed, err, checksum_ok, checksum_err))
    
    return results


def _collect_all_results(check_checksums: bool, timeout: float) -> List[UrlCheckResult]:
    """Collect verification results for all projects."""
    results = []
    seen_projects = set()
    
    for name, proj in iter_projects():
        if id(proj) in seen_projects:
            continue
        seen_projects.add(id(proj))
        results.extend(_check_project_urls(name, proj, check_checksums, timeout))
    
    return results


def _print_verbose_output(results: List[UrlCheckResult], fail_avail: List[UrlCheckResult], fail_checksum: List[UrlCheckResult]):
    """Print detailed output showing all URLs."""
    print("Project                             Status  Checksum URL")
    print("-" * 100)
    for r in results:
        mark = 'OK' if r.ok else 'FAIL'
        checksum_status = ''
        if r.checksum_ok is True:
            checksum_status = 'OK'
        elif r.checksum_ok is False:
            checksum_status = 'FAIL'
        elif r.checksum_ok is None and r.checksum_error:
            checksum_status = 'ERR'
        else:
            checksum_status = '-'
        
        print(f"{r.project:30} {mark:5} {r.status or '-':6} {checksum_status:8} {r.url}")
        
        # Print error details on the next line if there are checksum errors
        if r.checksum_ok is False and r.checksum_error:
            print(f"{'':30} {'':5} {'':6} {'':8} -> {r.checksum_error}")
    
    if fail_avail:
        print(f"\nAvailability failures: {len(fail_avail)}")
    if fail_checksum:
        print(f"Checksum failures: {len(fail_checksum)}")


def _print_brief_output(fail_avail: List[UrlCheckResult], fail_checksum: List[UrlCheckResult]):
    """Print brief output showing only broken URLs."""
    if fail_avail:
        print("Broken URLs (availability failures):")
        for r in fail_avail:
            print(f"{r.project:30} {r.status or '-':6} {r.url} {r.error or ''}")
    
    if fail_checksum:
        print("Broken URLs (checksum failures):")
        for r in fail_checksum:
            print(f"{r.project:30} {r.status or '-':6} {r.url} (checksum FAIL: {r.checksum_error})")
    
    if not fail_avail and not fail_checksum:
        print("No broken URLs found.")


def main():
    ap = argparse.ArgumentParser(description="Verify source URLs from libs.py")
    ap.add_argument('--checksum', action='store_true', help='Also verify checksums (downloads full archives)')
    ap.add_argument('--timeout', type=float, default=DEFAULT_TIMEOUT, help=f'Per-URL timeout seconds (default {DEFAULT_TIMEOUT})')
    ap.add_argument('--json', action='store_true', help='Output machine-readable JSON')
    ap.add_argument('--verbose', action='store_true', help='Show all URLs, not just broken ones')
    args = ap.parse_args()

    results = _collect_all_results(args.checksum, args.timeout)
    fail_avail = [r for r in results if not r.ok]
    fail_checksum = [r for r in results if r.checksum_ok is False]

    if args.json:
        print(json.dumps([r.__dict__ for r in results], indent=2))
    elif args.verbose:
        _print_verbose_output(results, fail_avail, fail_checksum)
    else:
        _print_brief_output(fail_avail, fail_checksum)

    # Exit with code 1 if any failures occurred, 0 otherwise
    if fail_avail or fail_checksum:
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    try:
        main()
    except KeyboardInterrupt:
        print("Interrupted", file=sys.stderr)
        sys.exit(130)
    except Exception:
        traceback.print_exc()
        sys.exit(99)
