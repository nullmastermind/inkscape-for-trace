#!/usr/bin/env bash
# =============================================================================
# release.sh - Inkscape Release Build Script for Ubuntu/WSL Ubuntu
# =============================================================================
# This script performs a full release build of the Inkscape fork.
# It automatically installs missing dependencies and builds the project.
#
# Usage: ./release.sh
#
# Requirements:
#   - Ubuntu or WSL Ubuntu environment
#   - sudo privileges (for package installation)
#   - git (must be pre-installed)
# =============================================================================

set -euo pipefail  # Exit on error, undefined vars, pipe failures
IFS=$'\n\t'

# Enable debug mode if RELEASE_DEBUG=1
if [[ "${RELEASE_DEBUG:-0}" == "1" ]]; then
    set -x
fi

# =============================================================================
# Configuration
# =============================================================================
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"
OUTPUT_DIR="${SCRIPT_DIR}/release-output"
BUILD_TYPE="Release"
PARALLEL_JOBS=$(nproc 2>/dev/null || echo 4)

# =============================================================================
# Logging Functions
# =============================================================================
log_info()    { echo -e "\033[1;34m[INFO]\033[0m    $1"; }
log_success() { echo -e "\033[1;32m[SUCCESS]\033[0m $1"; }
log_warning() { echo -e "\033[1;33m[WARNING]\033[0m $1"; }
log_error()   { echo -e "\033[1;31m[ERROR]\033[0m   $1" >&2; }
log_step()    { echo -e "\n\033[1;36m==>\033[0m \033[1m$1\033[0m"; }

# =============================================================================
# Environment Detection
# =============================================================================
detect_environment() {
    log_step "Detecting environment..."

    # Check for /etc/os-release
    if [[ ! -f /etc/os-release ]]; then
        log_error "Cannot determine distribution. /etc/os-release not found."
        exit 1
    fi

    source /etc/os-release

    # Check if running Ubuntu, Debian, or derivatives
    if [[ "$ID" != "ubuntu" && "$ID" != "debian" && "$ID_LIKE" != *"ubuntu"* && "$ID_LIKE" != *"debian"* ]]; then
        log_error "This script only supports Ubuntu, Debian, and WSL Ubuntu/Debian."
        log_error "Detected: $ID (ID_LIKE: ${ID_LIKE:-none})"
        exit 1
    fi

    # Detect WSL
    IS_WSL=false
    if grep -qi microsoft /proc/version 2>/dev/null || [[ -n "${WSL_DISTRO_NAME:-}" ]]; then
        IS_WSL=true
        log_info "Detected: WSL $ID ($VERSION_CODENAME)"
    else
        log_info "Detected: $ID ($VERSION_CODENAME)"
    fi

    log_success "Environment check passed"
}

# =============================================================================
# Dependency Installation
# =============================================================================
install_dependencies() {
    log_step "Installing build dependencies..."

    # Update package lists
    log_info "Updating package lists..."
    sudo apt-get update -yqq

    # Core build tools
    log_info "Installing core build tools..."
    sudo apt-get install -y -qq \
        build-essential \
        cmake \
        ninja-build \
        intltool \
        pkg-config \
        python3-dev \
        libtool \
        ccache \
        git \
        dos2unix \
        perl

    # Required libraries
    log_info "Installing required libraries..."
    sudo apt-get install -y -qq \
        wget \
        libart-2.0-dev \
        libblas3 \
        liblapack3 \
        libboost-dev \
        libboost-filesystem-dev \
        libboost-stacktrace-dev \
        libcdr-dev \
        libdouble-conversion-dev \
        libgc-dev \
        libglib2.0-dev \
        libgsl-dev \
        libgtk-3-dev \
        libgtkmm-3.0-dev \
        libgspell-1-dev \
        libharfbuzz-dev \
        liblcms2-dev \
        libmagick++-dev \
        libpango1.0-dev \
        libpng-dev \
        libpoppler-glib-dev \
        libpoppler-private-dev \
        libpotrace-dev \
        libreadline-dev \
        librevenge-dev \
        libsigc++-2.0-dev \
        libsoup2.4-dev \
        libvisio-dev \
        libwpg-dev \
        libxml-parser-perl \
        libxml2-dev \
        libxslt1-dev \
        zlib1g-dev

    # Optional dependencies (may fail on older Ubuntu versions)
    log_info "Installing optional dependencies..."
    sudo apt-get install -y -qq \
        libgtksourceview-4-dev \
        python3-lxml \
        || log_warning "Some optional packages not available (this is OK)"

    # Python runtime dependencies for extensions
    log_info "Installing Python dependencies for extensions..."
    sudo apt-get install -y -qq \
        python3-pip \
        python3-cssselect \
        python3-numpy \
        python3-pil \
        python3-lxml \
        python3-serial \
        python3-scour \
        || log_warning "Some Python packages not available"

    log_success "Dependencies installed successfully"
}

# =============================================================================
# Git Submodules
# =============================================================================
init_submodules() {
    log_step "Initializing git submodules..."

    cd "$SCRIPT_DIR"

    if [[ -f .gitmodules ]]; then
        git submodule update --init --recursive || {
            log_warning "Submodule init failed (may be OK if no submodules)"
        }
        log_success "Submodules initialized"
    else
        log_info "No .gitmodules found, skipping"
    fi
}

# =============================================================================
# Fix Line Endings (CRLF -> LF)
# =============================================================================
fix_line_endings() {
    log_step "Fixing CRLF line endings in build scripts..."

    cd "$SCRIPT_DIR"

    # Check if dos2unix is available
    if ! command -v dos2unix &>/dev/null; then
        log_warning "dos2unix not found, skipping line ending conversion"
        log_warning "Install with: sudo apt-get install dos2unix"
        return 0
    fi

    # List of scripts that are executed during the build process
    # These must have Unix line endings (LF) to work on Linux/WSL
    local build_scripts=(
        # Man page generation scripts (Perl)
        "man/fix-roff-punct"
        "man/utf8-to-roff"
        # PO/translation scripts
        "po/check-markup"
        "po/generate_POTFILES.sh"
        "po/check_for_tutorial_problems.sh"
        # Build tools
        "buildtools/clangtidy-helper.sh"
        "buildtools/check_license_headers.py"
        # Root scripts
        "download-gtest.sh"
    )

    local converted=0
    local skipped=0

    for script in "${build_scripts[@]}"; do
        if [[ -f "$script" ]]; then
            # Check if file has CRLF line endings using grep for \r
            # Use || true to prevent exit on no match
            if grep -q $'\r' "$script" 2>/dev/null; then
                log_info "Converting: $script"
                dos2unix -q "$script" 2>/dev/null || true
                converted=$((converted + 1))
            else
                skipped=$((skipped + 1))
            fi
        fi
    done

    # Also scan for any other executable scripts in key directories
    # that might have CRLF issues
    log_info "Scanning for additional scripts with CRLF endings..."

    local script
    local find_output

    # Find scripts with extensions
    find_output=$(find "$SCRIPT_DIR/man" "$SCRIPT_DIR/po" "$SCRIPT_DIR/buildtools" \
        -type f \( -name "*.sh" -o -name "*.pl" -o -name "*.py" \) 2>/dev/null || true)

    if [[ -n "$find_output" ]]; then
        while IFS= read -r script; do
            [[ -z "$script" ]] && continue
            if grep -q $'\r' "$script" 2>/dev/null; then
                log_info "Converting: $script"
                dos2unix -q "$script" 2>/dev/null || true
                converted=$((converted + 1))
            fi
        done <<< "$find_output"
    fi

    # Handle scripts without extensions but with shebang
    find_output=$(find "$SCRIPT_DIR/man" "$SCRIPT_DIR/po" \
        -maxdepth 1 -type f ! -name "*.*" 2>/dev/null || true)

    if [[ -n "$find_output" ]]; then
        while IFS= read -r script; do
            [[ -z "$script" ]] && continue
            [[ ! -f "$script" ]] && continue
            # Check for shebang - use || true to handle grep failure
            if head -1 "$script" 2>/dev/null | grep -q "^#!" 2>/dev/null || false; then
                if grep -q $'\r' "$script" 2>/dev/null; then
                    log_info "Converting: $script"
                    dos2unix -q "$script" 2>/dev/null || true
                    converted=$((converted + 1))
                fi
            fi
        done <<< "$find_output"
    fi

    # Print summary
    if [[ $converted -gt 0 ]]; then
        log_success "Converted $converted file(s) from CRLF to LF"
    else
        log_info "No CRLF line ending conversions needed ($skipped file(s) already had LF)"
    fi

    return 0
}

# =============================================================================
# Build Functions
# =============================================================================
configure_build() {
    log_step "Configuring build with CMake..."

    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    # Determine generator (prefer Ninja if available)
    local generator="Unix Makefiles"
    local build_cmd="make -j${PARALLEL_JOBS}"

    if command -v ninja &>/dev/null; then
        generator="Ninja"
        build_cmd="ninja"
        log_info "Using Ninja build system"
    else
        log_info "Using Make build system"
    fi

    # Configure with CMake
    log_info "Running cmake configuration..."
    cmake \
        -G "$generator" \
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
        -DCMAKE_INSTALL_PREFIX="$OUTPUT_DIR" \
        -DCMAKE_C_COMPILER_LAUNCHER=ccache \
        -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
        ..

    log_success "CMake configuration complete"
}

build_project() {
    log_step "Building Inkscape ($BUILD_TYPE)..."

    cd "$BUILD_DIR"

    # Determine build command
    if [[ -f build.ninja ]]; then
        ninja
    else
        make -j"${PARALLEL_JOBS}"
    fi

    log_success "Build completed successfully"
}

install_artifacts() {
    log_step "Installing artifacts to output directory..."

    cd "$BUILD_DIR"

    # Create output directory
    mkdir -p "$OUTPUT_DIR"

    # Run make install
    if [[ -f build.ninja ]]; then
        ninja install
    else
        make install
    fi

    log_success "Artifacts installed to: $OUTPUT_DIR"
}

# =============================================================================
# Verification
# =============================================================================
verify_build() {
    log_step "Verifying build artifacts..."

    local inkscape_bin="$BUILD_DIR/bin/inkscape"

    if [[ -x "$inkscape_bin" ]]; then
        log_info "Testing inkscape binary..."
        "$inkscape_bin" --version && log_success "Inkscape binary works!"
    else
        log_warning "Inkscape binary not found at expected location"
        # Check alternative locations
        if [[ -x "$OUTPUT_DIR/bin/inkscape" ]]; then
            log_info "Found in output directory"
            "$OUTPUT_DIR/bin/inkscape" --version
        fi
    fi
}

# =============================================================================
# Summary
# =============================================================================
print_summary() {
    log_step "Build Summary"

    echo ""
    echo "=============================================="
    echo "  INKSCAPE RELEASE BUILD COMPLETE"
    echo "=============================================="
    echo ""
    echo "  Build Type:     $BUILD_TYPE"
    echo "  Build Dir:      $BUILD_DIR"
    echo "  Output Dir:     $OUTPUT_DIR"
    echo ""
    echo "  Binary locations:"
    echo "    - Build:      $BUILD_DIR/bin/inkscape"
    echo "    - Installed:  $OUTPUT_DIR/bin/inkscape"
    echo ""
    echo "  To run Inkscape:"
    echo "    $BUILD_DIR/bin/inkscape"
    echo ""
    echo "  To use the selection-trace action:"
    echo "    $BUILD_DIR/bin/inkscape --actions=\"select-all;selection-trace:256,false,true,true,4,1.0,0.20;export-filename:output.svg;export-do;\" input.png --batch-process"
    echo ""
    echo "=============================================="
}

# =============================================================================
# Main
# =============================================================================
main() {
    log_step "Starting Inkscape Release Build"
    log_info "Script directory: $SCRIPT_DIR"
    log_info "Using $PARALLEL_JOBS parallel jobs"

    detect_environment
    install_dependencies
    init_submodules
    fix_line_endings
    configure_build
    build_project
    install_artifacts
    verify_build
    print_summary

    log_success "Release build completed successfully!"
    exit 0
}

# Run main function
main "$@"