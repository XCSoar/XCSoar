#!/usr/bin/env python3
"""Run XCSoar unit tests inside an iOS Simulator.

This script builds simulator artifacts, installs XCSoar.app, and executes
selected test binaries using simctl spawn.
"""

from __future__ import annotations

import json
import os
import plistlib
import shutil
import subprocess
import sys
from pathlib import Path


def run(cmd: list[str], check: bool = True, capture_output: bool = False) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        cmd,
        check=check,
        text=True,
        capture_output=capture_output,
    )


def cpu_count() -> int:
    return os.cpu_count() or 4


def find_simulator_udid(device_name: str) -> str:
    proc = run(["xcrun", "simctl", "list", "devices", "available", "--json"], capture_output=True)
    payload = json.loads(proc.stdout)

    matches: list[tuple[bool, str, str]] = []
    for runtime, devices in payload.get("devices", {}).items():
        for device in devices:
            if not device.get("isAvailable"):
                continue
            if device.get("name") != device_name:
                continue

            udid = device.get("udid")
            if not udid:
                continue

            is_booted = device.get("state", "") == "Booted"
            matches.append((is_booted, runtime, udid))

    if not matches:
        return ""

    # Prefer already booted device; otherwise prefer highest runtime key.
    matches.sort(key=lambda item: (item[0], item[1]))
    return matches[-1][2]


def collect_all_test_names(bin_dir: Path) -> list[str]:
    names: list[str] = []
    for path in sorted(bin_dir.iterdir()):
        if not path.is_file():
            continue
        if not (path.name.startswith("test_") or path.name.startswith("Test")):
            continue
        if not os.access(path, os.X_OK):
            continue
        names.append(path.name)
    return names


def write_wrapper(wrapper_path: Path, udid: str, test_path: str) -> None:
    wrapper_path.write_text(
        "#!/bin/sh\n"
        "set -eu\n"
        f"exec xcrun simctl spawn \"{udid}\" \"{test_path}\"\n",
        encoding="utf-8",
    )
    wrapper_path.chmod(0o755)


def find_simulator_data_root(container_path: Path) -> Path | None:
    """Return the simulator '.../data' root for a simctl container path.

    Expected shape includes a 'Containers' segment, e.g.:
        .../data/Containers/Bundle/Application/<uuid>/XCSoar.app
        .../data/Containers/Data/Application/<uuid>
    """

    for candidate in (container_path, *container_path.parents):
        if candidate.name == "Containers":
            return candidate.parent

    return None


def ensure_symlink(link_path: Path, target_path: Path) -> None:
    """Ensure link_path is a symlink pointing to target_path."""

    try:
        if link_path.is_symlink() and link_path.resolve() == target_path.resolve():
            return
    except FileNotFoundError:
        # Dangling symlink; remove and recreate below.
        pass

    if link_path.exists() or link_path.is_symlink():
        if link_path.is_dir() and not link_path.is_symlink():
            shutil.rmtree(link_path)
        else:
            link_path.unlink()

    link_path.symlink_to(target_path, target_is_directory=True)


def main() -> int:
    root_dir = Path(__file__).resolve().parent.parent
    os.chdir(root_dir)

    make_bin = os.environ.get("MAKE_BIN", "gmake")
    target = os.environ.get("TARGET", "IOS64SIM")
    sim_device_name = os.environ.get("SIM_DEVICE_NAME", "iPhone 16 Pro")
    sim_tests_mode = os.environ.get("SIM_TESTS_MODE", "all")
    smoke_tests_env = os.environ.get("SIM_SMOKE_TESTS", "TestCRC8 TestCRC16 TestHexString")
    sim_skip_tests_env = os.environ.get("SIM_SKIP_TESTS", "TestWrapText")

    target_output_dir = root_dir / "output" / target
    bin_dir = target_output_dir / "bin"
    app_path = target_output_dir / "ipa" / "Payload" / "XCSoar.app"
    app_test_dir = app_path / "tests"

    print(f"check-ios-sim: building simulator app and tests for target {target}")
    run([make_bin, f"-j{cpu_count()}", f"TARGET={target}", "build-check", "ipa"])

    if not app_path.is_dir():
        print(f"Error: app bundle not found at {app_path}", file=sys.stderr)
        return 1

    if not bin_dir.is_dir():
        print(f"Error: test binaries directory not found at {bin_dir}", file=sys.stderr)
        return 1

    if sim_tests_mode == "all":
        test_names = collect_all_test_names(bin_dir)
    elif sim_tests_mode == "smoke":
        test_names = [name for name in smoke_tests_env.split() if name]
    else:
        print(f"Error: unsupported SIM_TESTS_MODE '{sim_tests_mode}' (use 'smoke' or 'all')", file=sys.stderr)
        return 1

    if not test_names:
        print("Error: no test binaries selected", file=sys.stderr)
        return 1

    skip_tests = {name for name in sim_skip_tests_env.split() if name}
    if skip_tests:
        original_count = len(test_names)
        test_names = [name for name in test_names if name not in skip_tests]
        skipped = original_count - len(test_names)
        if skipped > 0:
            print(f"check-ios-sim: skipping {skipped} test(s): {' '.join(sorted(skip_tests))}")

    if not test_names:
        print("Error: no tests left after applying SIM_SKIP_TESTS", file=sys.stderr)
        return 1

    device_udid = find_simulator_udid(sim_device_name)
    if not device_udid:
        print(f"Error: could not find an available simulator named '{sim_device_name}'", file=sys.stderr)
        print("Hint: set SIM_DEVICE_NAME to one from 'xcrun simctl list devices available --json'", file=sys.stderr)
        return 1

    print(f"check-ios-sim: using simulator {sim_device_name} ({device_udid})")
    run(["xcrun", "simctl", "boot", device_udid], check=False)
    run(["xcrun", "simctl", "bootstatus", device_udid, "-b"])

    # Keep tests inside the source app bundle too, useful for local inspection.
    if app_test_dir.exists():
        shutil.rmtree(app_test_dir)
    app_test_dir.mkdir(parents=True, exist_ok=True)

    for name in test_names:
        src = bin_dir / name
        if not (src.is_file() and os.access(src, os.X_OK)):
            print(f"Error: test binary not found or not executable: {src}", file=sys.stderr)
            return 1

        dst = app_test_dir / name
        shutil.copy2(src, dst)
        dst.chmod(0o755)

    plist_path = app_path / "Info.plist"
    with plist_path.open("rb") as fp:
        info = plistlib.load(fp)
    bundle_id = info.get("CFBundleIdentifier")
    if not bundle_id:
        print(f"Error: CFBundleIdentifier missing in {plist_path}", file=sys.stderr)
        return 1

    print(f"check-ios-sim: installing app bundle {bundle_id}")
    run(["xcrun", "simctl", "install", device_udid, str(app_path)])

    container_app_path = run(
        ["xcrun", "simctl", "get_app_container", device_udid, bundle_id, "app"],
        capture_output=True,
    ).stdout.strip()

    container_data_path = run(
        ["xcrun", "simctl", "get_app_container", device_udid, bundle_id, "data"],
        capture_output=True,
    ).stdout.strip()

    if not container_app_path:
        print("Error: failed to resolve installed app container path", file=sys.stderr)
        return 1

    if not container_data_path:
        print("Error: failed to resolve installed app data container path", file=sys.stderr)
        return 1

    # Tests are launched by simctl spawn with cwd at <simulator data root>.
    # Resolve that root by finding the 'Containers' anchor in the container
    # path returned by simctl instead of relying on fragile parent depths.
    installed_app_dir = Path(container_app_path)
    installed_data_dir = Path(container_data_path)
    if not installed_data_dir.is_dir():
        print(f"Error: installed app data container path does not exist: {installed_data_dir}", file=sys.stderr)
        return 1

    device_data_root = find_simulator_data_root(installed_data_dir)
    if device_data_root is None:
        print(
            f"Error: could not derive simulator data root from path: {installed_data_dir}",
            file=sys.stderr,
        )
        return 1

    output_root = device_data_root / "output"
    for required_dir in (output_root, output_root / "results", output_root / "test"):
        required_dir.mkdir(parents=True, exist_ok=True)

    sim_test_root_link = device_data_root / "test"
    repo_test_root = root_dir / "test"
    ensure_symlink(sim_test_root_link, repo_test_root)

    # On install, simulator may reset file mode bits in app resources.
    # Re-copy tests directly into the installed app container to enforce +x.
    installed_tests_dir = installed_app_dir / "tests"
    installed_tests_dir.mkdir(parents=True, exist_ok=True)
    for name in test_names:
        src = bin_dir / name
        dst = installed_tests_dir / name
        shutil.copy2(src, dst)
        dst.chmod(0o755)

    wrappers_dir = target_output_dir / "ios-sim-tap-wrappers"
    if wrappers_dir.exists():
        shutil.rmtree(wrappers_dir)
    wrappers_dir.mkdir(parents=True, exist_ok=True)

    wrapper_paths: list[Path] = []
    for name in test_names:
        test_path = f"{container_app_path}/tests/{name}"
        wrapper_path = wrappers_dir / name
        write_wrapper(wrapper_path, device_udid, test_path)
        wrapper_paths.append(wrapper_path)

    harness_path = root_dir / "test" / "src" / "testall.pl"
    print(f"check-ios-sim: running {len(wrapper_paths)} test binary wrappers through TAP harness")
    rc = run(["perl", str(harness_path), *[str(p) for p in wrapper_paths]], check=False).returncode
    if rc != 0:
        print("check-ios-sim: TAP harness reported failures", file=sys.stderr)
        return rc

    print("check-ios-sim: all tests passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
