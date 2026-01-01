# Build All Configurations Script

This script builds XCSoar with every target and flag configuration combination.

## Usage

### Basic Usage

Build all targets with all flag combinations:
```bash
./build_all_configs.sh
```

### Advanced Usage

Build only specific targets:
```bash
./build_all_configs.sh --only-target UNIX --only-target PC
```

Skip specific targets:
```bash
./build_all_configs.sh --skip-target ANDROID --skip-target IOS32
```

Build only with specific flags:
```bash
./build_all_configs.sh --only-flags "DEBUG=y"
```

Skip builds with specific flags:
```bash
./build_all_configs.sh --skip-flags "LTO=y"
```

Dry run (see what would be built without actually building):
```bash
./build_all_configs.sh --dry-run
```

## Targets

The script builds the following targets:
- PC, WIN64
- UNIX, UNIX32, UNIX64, OPT
- WAYLAND
- FUZZER
- PI, PI2, CUBIE, KOBO, NEON
- ANDROID, ANDROID7, ANDROID86, ANDROIDAARCH64, ANDROIDX64, ANDROIDFAT
- OSX64, MACOS
- IOS32, IOS64, IOS64SIM

## Flag Combinations

The script tests the following flag combinations:
- Default debug build (`DEBUG=y`)
- Release build (`DEBUG=n`)
- With LTO (`LTO=y`)
- With ThinLTO (`THIN_LTO=y`)
- With Clang (`CLANG=y`)
- With sanitizers (`SANITIZE=address`)
- Headless build (`HEADLESS=y`)
- SDL builds (`ENABLE_SDL=y`)
- OpenGL builds (`OPENGL=y`)
- GLES2 builds (`GLES2=y`)
- Greyscale builds (`GREYSCALE=y`)
- Dither builds (`DITHER=y`)
- VFB builds (`VFB=y`)
- Frame buffer builds (`USE_FB=y`)
- MESA KMS builds (`ENABLE_MESA_KMS=y`)
- Without eye candy (`EYE_CANDY=n`)
- With ICF (`ICF=y`)
- With ccache (`USE_CCACHE=y`)

## Output

The script creates a `build_logs/` directory with:
- Individual log files for each build (`<target>_<flags>.log`)
- `build_results.txt` - All results
- `successful_builds.txt` - Only successful builds
- `failed_builds.txt` - Only failed builds

## Examples

### Build only UNIX targets with default flags
```bash
./build_all_configs.sh --only-target UNIX --only-flags ""
```

### Build all targets except Android, with only debug builds
```bash
./build_all_configs.sh --skip-target ANDROID --skip-target ANDROID7 --skip-target ANDROID86 --skip-target ANDROIDAARCH64 --skip-target ANDROIDX64 --skip-target ANDROIDFAT --only-flags "DEBUG=y"
```

### Test a specific configuration
```bash
./build_all_configs.sh --only-target UNIX --only-flags "DEBUG=n LTO=y"
```

## Notes

- The script automatically skips invalid flag combinations for specific targets (e.g., SDL flags for Windows targets)
- Build logs are saved for debugging failed builds
- The script provides a summary at the end with success/failure counts
- Failed builds are logged separately for easy review

## Performance

Building all configurations can take a very long time. Consider:
- Using `--only-target` to test specific targets first
- Using `--skip-flags` to skip time-consuming builds (like LTO)
- Running on a machine with many CPU cores
- Using `USE_CCACHE=y` to speed up repeated builds



