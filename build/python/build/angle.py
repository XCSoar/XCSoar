import glob
import os
import os.path
import shutil
import subprocess
import tempfile
from typing import Optional

from build.download import download_and_verify, download_basename
from build.project import Project
from build.tar import untar
from .toolchain import AnyToolchain


class AngleProject(Project):
    RELEASE_TAG = "a96fca8"
    RELEASE_REVISION = "a96fca8d5ee2ca61e8de419e38cd577579281c9e"
    RELEASE_BASE_URL = (
        "https://github.com/yorickreum/angle-libs/releases/download/" + RELEASE_TAG
    )

    DIGESTS = {
        ("darwin", "arm64"): "c8d1f7e51ae89a21cf4caafd0405430dce498e97f9080cdf9c1e265cccb0148e",
        ("darwin", "x64"): "2f619e06bcdddc1e3e6fdfdf1abd3d7ada67483780d17577b668247ab6eeb266",
        ("windows", "arm64"): "f7f122533644ec0dd1f52f0117b9fa57ceb3f69dff3359579028ff4008f74f69",
        ("windows", "x64"): "e0e195e9b8ac3e6dd50418d2284ab0c3803d7587c7473a2fa97cbd373d15e650",
        ("windows", "x86"): "0d2302e1d365caa2e0dea63a0e518e3a6294d9fa1e6f9c0e195359f85c4090c5",
    }

    def __init__(self) -> None:
        sample_base = "angle-macos-arm64-" + self.RELEASE_REVISION
        sample_url = self.RELEASE_BASE_URL + "/" + sample_base + ".tar.gz"
        Project.__init__(
            self,
            sample_url,
            self.DIGESTS[("darwin", "arm64")],
            "include/EGL/egl.h",
            name="angle",
            version=self.RELEASE_TAG,
            base=sample_base,
        )

    def _resolve_platform(self, toolchain: AnyToolchain) -> tuple[str, str]:
        if toolchain.is_darwin:
            if toolchain.is_aarch64:
                return "darwin", "arm64"
            return "darwin", "x64"

        if toolchain.is_windows:
            if toolchain.is_aarch64:
                return "windows", "arm64"
            if toolchain.host_triplet.startswith("i686-"):
                return "windows", "x86"
            if toolchain.host_triplet.startswith("x86_64-"):
                return "windows", "x64"
            raise RuntimeError(
                f"Unsupported Windows target architecture for ANGLE: {toolchain.host_triplet}"
            )

        raise RuntimeError("ANGLE is only available for Darwin and Windows targets")

    def _asset_data(self, toolchain: AnyToolchain) -> tuple[str, str, str]:
        platform, arch = self._resolve_platform(toolchain)
        os_platform = "macos" if platform == "darwin" else "windows"
        base = f"angle-{os_platform}-{arch}-{self.RELEASE_REVISION}"
        digest = self.DIGESTS[(platform, arch)]
        url = self.RELEASE_BASE_URL + "/" + base + ".tar.gz"
        return base, url, digest

    def download(self, toolchain: AnyToolchain) -> str:
        _base, url, digest = self._asset_data(toolchain)
        return download_and_verify(url, digest, toolchain.tarball_path)

    def _cached_tarball(self, toolchain: AnyToolchain) -> str:
        _base, url, _digest = self._asset_data(toolchain)
        return os.path.join(toolchain.tarball_path, download_basename(url))

    def is_installed(self, toolchain: AnyToolchain) -> bool:
        tarball = self._cached_tarball(toolchain)
        if not os.path.isfile(tarball):
            return False

        tarball_mtime = os.path.getmtime(tarball)

        include_dir = os.path.join(toolchain.install_prefix, "include")
        lib_dir = os.path.join(toolchain.install_prefix, "lib")
        required = [
            os.path.join(include_dir, "EGL", "egl.h"),
            os.path.join(include_dir, "GLES2", "gl2.h"),
        ]

        if toolchain.is_darwin:
            required += [
                os.path.join(lib_dir, "libEGL.dylib"),
                os.path.join(lib_dir, "libGLESv2.dylib"),
            ]
        elif toolchain.is_windows:
            bin_dir = os.path.join(toolchain.install_prefix, "bin")
            required += [
                os.path.join(bin_dir, "libEGL.dll"),
                os.path.join(bin_dir, "libGLESv2.dll"),
                os.path.join(lib_dir, "libEGL.dll.a"),
                os.path.join(lib_dir, "libGLESv2.dll.a"),
            ]
        else:
            return False # Unsupported platform

        try:
            return all(os.path.getmtime(path) >= tarball_mtime for path in required)
        except FileNotFoundError:
            return False

    def _find_program(self, names: list[str]) -> Optional[str]:
        for name in names:
            path = shutil.which(name)
            if path is not None:
                return path
        return None

    def _generate_import_library(
        self,
        toolchain: AnyToolchain,
        dll_path: str,
        import_lib_path: str,
    ) -> None:
        dlltool = self._find_program(
            [f"{toolchain.host_triplet}-dlltool", "dlltool", "llvm-dlltool"]
        )
        gendef = self._find_program([f"{toolchain.host_triplet}-gendef", "gendef"])
        if dlltool is None or gendef is None:
            raise RuntimeError(
                "Generating Windows ANGLE import libraries requires 'dlltool' and "
                "'gendef' in PATH"
            )

        dll_name = os.path.basename(dll_path)
        base_name = os.path.splitext(dll_name)[0]

        with tempfile.TemporaryDirectory() as tmp:
            subprocess.check_call([gendef, dll_path], cwd=tmp, timeout=60)

            def_path = os.path.join(tmp, base_name + ".def")
            if not os.path.isfile(def_path):
                defs = glob.glob(os.path.join(tmp, "*.def"))
                if len(defs) != 1:
                    raise RuntimeError(f"Failed to find .def file for {dll_name}")
                def_path = defs[0]

            subprocess.check_call(
                [
                    dlltool,
                    "-d",
                    def_path,
                    "-l",
                    import_lib_path,
                    "-D",
                    dll_name,
                ],
                timeout=60,
            )

    def _build(self, toolchain: AnyToolchain) -> None:
        base, _url, _digest = self._asset_data(toolchain)
        src = untar(self.download(toolchain), toolchain.src_path, base, lazy=False)

        include_dir = os.path.join(toolchain.install_prefix, "include")
        lib_dir = os.path.join(toolchain.install_prefix, "lib")
        os.makedirs(include_dir, exist_ok=True)
        os.makedirs(lib_dir, exist_ok=True)

        shutil.copytree(os.path.join(src, "include"), include_dir, dirs_exist_ok=True)

        if toolchain.is_darwin:
            shutil.copy2(
                os.path.join(src, "libEGL.dylib"),
                os.path.join(lib_dir, "libEGL.dylib"),
            )
            shutil.copy2(
                os.path.join(src, "libGLESv2.dylib"),
                os.path.join(lib_dir, "libGLESv2.dylib"),
            )
            return

        if toolchain.is_windows:
            bin_dir = os.path.join(toolchain.install_prefix, "bin")
            os.makedirs(bin_dir, exist_ok=True)

            for name in ("libEGL.dll", "libGLESv2.dll"):
                shutil.copy2(os.path.join(src, name), os.path.join(bin_dir, name))

            for base_name in ("libEGL", "libGLESv2"):
                src_import_lib = os.path.join(src, base_name + ".dll.a")
                dst_import_lib = os.path.join(lib_dir, base_name + ".dll.a")

                if os.path.isfile(src_import_lib):
                    shutil.copyfile(src_import_lib, dst_import_lib)
                else:
                    self._generate_import_library(
                        toolchain,
                        os.path.join(bin_dir, base_name + ".dll"),
                        dst_import_lib,
                    )
            return

        raise RuntimeError("Unsupported target for ANGLE")
