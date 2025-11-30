#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Build script for XCSoar that builds every target and flag configuration

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# If script is in project root, use script dir; if in a subdirectory, go up one level
if [[ -f "$SCRIPT_DIR/Makefile" ]]; then
    PROJECT_ROOT="$SCRIPT_DIR"
else
    PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
fi
LOG_DIR="${PROJECT_ROOT}/build_logs"
RESULTS_FILE="${LOG_DIR}/build_results.txt"
FAILED_BUILDS="${LOG_DIR}/failed_builds.txt"
SUCCESSFUL_BUILDS="${LOG_DIR}/successful_builds.txt"

# Create log directory
mkdir -p "$LOG_DIR"

# Initialize result files
> "$RESULTS_FILE"
> "$FAILED_BUILDS"
> "$SUCCESSFUL_BUILDS"

# Counters
TOTAL_BUILDS=0
SUCCESSFUL=0
FAILED=0
SKIPPED=0

# All targets from build/targets.mk
TARGETS=(
    "PC"
    "WIN64"
    "UNIX"
    "UNIX32"
    "UNIX64"
    "OPT"
    "WAYLAND"
    "FUZZER"
    "PI"
    "PI2"
    "CUBIE"
    "KOBO"
    "NEON"
    "ANDROID"
    "ANDROID7"
    "ANDROID86"
    "ANDROIDAARCH64"
    "ANDROIDX64"
    "ANDROIDFAT"
    "OSX64"
    "MACOS"
    "IOS32"
    "IOS64"
    "IOS64SIM"
)

# Common flag combinations to test
# Format: "FLAG1=value1 FLAG2=value2 ..."
FLAG_COMBINATIONS=(
    # Default debug build
    "DEBUG=y"
    
    # Release build
    "DEBUG=n"
    
    # With LTO
    "DEBUG=n LTO=y"
    
    # With ThinLTO
    "DEBUG=n THIN_LTO=y"
    
    # With Clang
    "CLANG=y"
    
    # With sanitizers
    "SANITIZE=address"
    
    # Headless build
    "HEADLESS=y"
    
    # SDL builds (for applicable targets)
    "ENABLE_SDL=y"
    
    # OpenGL builds
    "OPENGL=y"
    
    # GLES2 builds
    "GLES2=y"
    
    # Greyscale builds
    "GREYSCALE=y"
    
    # Dither builds
    "DITHER=y"
    
    # VFB builds
    "VFB=y"
    
    # Frame buffer builds
    "USE_FB=y"
    
    # MESA KMS builds
    "ENABLE_MESA_KMS=y"
    
    # Without eye candy
    "EYE_CANDY=n"
    
    # With ICF
    "ICF=y"
    
    # With ccache
    "USE_CCACHE=y"
)

# Flags that are target-specific and should be skipped for certain targets
TARGET_SPECIFIC_FLAGS=(
    "ENABLE_SDL"      # Only for UNIX-like targets
    "USE_FB"          # Only for Linux targets
    "VFB"             # Only for certain targets
    "ENABLE_MESA_KMS" # Only for Linux targets
    "GREYSCALE"       # Only for certain targets
    "DITHER"          # Only for certain targets
)

# Check if a flag combination is valid for a target
is_valid_combination() {
    local target=$1
    local flags=$2
    
    # Skip Android-specific flags for non-Android targets
    if [[ "$target" != ANDROID* ]] && [[ "$flags" == *"ANDROID"* ]]; then
        return 1
    fi
    
    # Skip iOS-specific flags for non-iOS targets
    if [[ "$target" != IOS* ]] && [[ "$flags" == *"IOS"* ]]; then
        return 1
    fi
    
    # Skip SDL for Windows/Android/iOS
    if [[ "$target" == "PC" || "$target" == "WIN64" || "$target" == ANDROID* || "$target" == IOS* ]]; then
        if [[ "$flags" == *"ENABLE_SDL=y"* ]]; then
            return 1
        fi
    fi
    
    # Skip FB/VFB for Windows/Android/iOS
    if [[ "$target" == "PC" || "$target" == "WIN64" || "$target" == ANDROID* || "$target" == IOS* ]]; then
        if [[ "$flags" == *"USE_FB=y"* || "$flags" == *"VFB=y"* ]]; then
            return 1
        fi
    fi
    
    # Skip MESA_KMS for non-Linux targets
    if [[ "$target" != UNIX* && "$target" != PI* && "$target" != CUBIE && "$target" != KOBO && "$target" != NEON ]]; then
        if [[ "$flags" == *"ENABLE_MESA_KMS=y"* ]]; then
            return 1
        fi
    fi
    
    # FUZZER target has specific requirements
    if [[ "$target" == "FUZZER" ]]; then
        # FUZZER already sets VFB=y, CLANG=y, etc.
        if [[ "$flags" == *"VFB=y"* || "$flags" == *"CLANG=y"* ]]; then
            return 1
        fi
    fi
    
    return 0
}

# Build a specific target with flags
build_target() {
    local target=$1
    local flags=$2
    local build_name="${target}"
    
    if [[ -n "$flags" ]]; then
        # Create a sanitized name for the build
        local flag_name=$(echo "$flags" | sed 's/ /_/g' | sed 's/=/-/g')
        build_name="${target}_${flag_name}"
    fi
    
    local log_file="${LOG_DIR}/${build_name}.log"
    local start_time=$(date +%s)
    
    echo -e "${BLUE}Building: ${build_name}${NC}"
    echo "Flags: ${flags:-default}"
    
    TOTAL_BUILDS=$((TOTAL_BUILDS + 1))
    
    # Change to project root
    cd "$PROJECT_ROOT"
    
    # Clean previous build artifacts (optional, comment out if you want incremental builds)
    # make clean >/dev/null 2>&1 || true
    
    # Build with make
    if make TARGET="$target" $flags > "$log_file" 2>&1; then
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        SUCCESSFUL=$((SUCCESSFUL + 1))
        echo -e "${GREEN}✓ SUCCESS${NC} (${duration}s)"
        echo "${build_name}|SUCCESS|${duration}s|${flags}" >> "$SUCCESSFUL_BUILDS"
        echo "${build_name}|SUCCESS|${duration}s|${flags}" >> "$RESULTS_FILE"
        return 0
    else
        local end_time=$(date +%s)
        local duration=$((end_time - start_time))
        FAILED=$((FAILED + 1))
        echo -e "${RED}✗ FAILED${NC} (${duration}s)"
        echo "  Log: $log_file"
        echo "${build_name}|FAILED|${duration}s|${flags}" >> "$FAILED_BUILDS"
        echo "${build_name}|FAILED|${duration}s|${flags}" >> "$RESULTS_FILE"
        return 1
    fi
}

# Print summary
print_summary() {
    echo ""
    echo "=========================================="
    echo "Build Summary"
    echo "=========================================="
    echo -e "Total builds attempted: ${TOTAL_BUILDS}"
    echo -e "${GREEN}Successful: ${SUCCESSFUL}${NC}"
    echo -e "${RED}Failed: ${FAILED}${NC}"
    echo -e "${YELLOW}Skipped: ${SKIPPED}${NC}"
    echo ""
    echo "Results saved to:"
    echo "  - $RESULTS_FILE"
    echo "  - $SUCCESSFUL_BUILDS"
    echo "  - $FAILED_BUILDS"
    echo ""
}

# Main execution
main() {
    echo "XCSoar Build All Configurations Script"
    echo "========================================"
    echo "Project root: $PROJECT_ROOT"
    echo "Log directory: $LOG_DIR"
    echo ""
    
    # Check if we're in the right directory
    if [[ ! -f "$PROJECT_ROOT/Makefile" ]]; then
        echo -e "${RED}Error: Makefile not found. Are you in the XCSoar project root?${NC}"
        exit 1
    fi
    
    # Parse command line arguments
    SKIP_TARGETS=()
    ONLY_TARGETS=()
    SKIP_FLAGS=()
    ONLY_FLAGS=()
    DRY_RUN=false
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --skip-target)
                SKIP_TARGETS+=("$2")
                shift 2
                ;;
            --only-target)
                ONLY_TARGETS+=("$2")
                shift 2
                ;;
            --skip-flags)
                SKIP_FLAGS+=("$2")
                shift 2
                ;;
            --only-flags)
                ONLY_FLAGS+=("$2")
                shift 2
                ;;
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo ""
                echo "Options:"
                echo "  --skip-target TARGET    Skip building this target"
                echo "  --only-target TARGET     Only build this target (can be used multiple times)"
                echo "  --skip-flags FLAGS       Skip builds with these flags"
                echo "  --only-flags FLAGS       Only build with these flags"
                echo "  --dry-run                Show what would be built without actually building"
                echo "  --help                   Show this help message"
                echo ""
                echo "Examples:"
                echo "  $0 --only-target UNIX"
                echo "  $0 --skip-target ANDROID --skip-target IOS32"
                echo "  $0 --only-flags 'DEBUG=y'"
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done
    
    if [[ "$DRY_RUN" == true ]]; then
        echo -e "${YELLOW}DRY RUN MODE - No builds will be executed${NC}"
        echo ""
    fi
    
    # Build each target
    for target in "${TARGETS[@]}"; do
        # Check if we should skip this target
        if [[ ${#SKIP_TARGETS[@]} -gt 0 ]]; then
            skip=false
            for skip_target in "${SKIP_TARGETS[@]}"; do
                if [[ "$target" == "$skip_target" ]]; then
                    skip=true
                    break
                fi
            done
            if [[ "$skip" == true ]]; then
                echo -e "${YELLOW}Skipping target: $target${NC}"
                SKIPPED=$((SKIPPED + 1))
                continue
            fi
        fi
        
        # Check if we should only build specific targets
        if [[ ${#ONLY_TARGETS[@]} -gt 0 ]]; then
            include=false
            for only_target in "${ONLY_TARGETS[@]}"; do
                if [[ "$target" == "$only_target" ]]; then
                    include=true
                    break
                fi
            done
            if [[ "$include" == false ]]; then
                echo -e "${YELLOW}Skipping target (not in --only-target list): $target${NC}"
                SKIPPED=$((SKIPPED + 1))
                continue
            fi
        fi
        
        echo ""
        echo "=========================================="
        echo "Target: $target"
        echo "=========================================="
        
        # Build with default flags first
        if [[ "$DRY_RUN" == false ]]; then
            build_target "$target" ""
        else
            echo "Would build: TARGET=$target (default flags)"
            TOTAL_BUILDS=$((TOTAL_BUILDS + 1))
        fi
        
        # Build with each flag combination
        for flags in "${FLAG_COMBINATIONS[@]}"; do
            # Check if we should skip these flags
            if [[ ${#SKIP_FLAGS[@]} -gt 0 ]]; then
                skip=false
                for skip_flag in "${SKIP_FLAGS[@]}"; do
                    if [[ "$flags" == *"$skip_flag"* ]]; then
                        skip=true
                        break
                    fi
                done
                if [[ "$skip" == true ]]; then
                    continue
                fi
            fi
            
            # Check if we should only build with specific flags
            if [[ ${#ONLY_FLAGS[@]} -gt 0 ]]; then
                include=false
                for only_flag in "${ONLY_FLAGS[@]}"; do
                    if [[ "$flags" == *"$only_flag"* ]]; then
                        include=true
                        break
                    fi
                done
                if [[ "$include" == false ]]; then
                    continue
                fi
            fi
            
            # Check if this combination is valid for this target
            if ! is_valid_combination "$target" "$flags"; then
                echo -e "${YELLOW}  Skipping invalid combination: $flags${NC}"
                SKIPPED=$((SKIPPED + 1))
                continue
            fi
            
            if [[ "$DRY_RUN" == false ]]; then
                build_target "$target" "$flags"
            else
                echo "Would build: TARGET=$target $flags"
                TOTAL_BUILDS=$((TOTAL_BUILDS + 1))
            fi
        done
    done
    
    if [[ "$DRY_RUN" == false ]]; then
        print_summary
    else
        echo ""
        echo "Dry run complete. Would attempt $TOTAL_BUILDS builds."
    fi
}

# Run main function
main "$@"

