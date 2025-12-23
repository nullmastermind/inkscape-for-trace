# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is a **modified fork of Inkscape** designed specifically for **automation scripts**, particularly for pixel art image tracing. The main custom feature is the `selection-trace` action with configurable parameters for batch processing.

## Build Commands

```bash
# Create build directory (first time)
mkdir build && cd build

# Configure with CMake (Linux with Ninja - recommended)
cmake -DCMAKE_INSTALL_PREFIX:PATH=$PWD/../ -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_BUILD_TYPE=Debug -G Ninja ..

# Build
ninja              # with Ninja
# or: make         # with Make

# Run from build directory
./bin/inkscape
```

### Build Options
```bash
cmake -L                           # List available options
cmake .. -DWITH_SVG2=OFF           # Disable SVG2 support
cmake -DCMAKE_BUILD_TYPE=Debug ..  # Debug build
cmake -DWITH_PROFILING=ON ..       # Enable profiling
```

## Testing

```bash
cd build
ctest -V                    # Run all tests
ctest -R test_name          # Run specific test (regex match)
ctest -N                    # List all tests without running
```

Tests use **Google Test (gtest)**. Test files are in `testfiles/src/` for unit tests, `testfiles/cli_tests/` for CLI tests, and `testfiles/rendering_tests/` for rendering comparisons.

## Custom Selection-Trace Action

**Location:** `src/actions/actions-object.cpp` (function `selection_trace`)

The fork adds a custom `selection-trace` action for command-line bitmap tracing:

```bash
# Action signature:
selection-trace:{scans},{is_smooth[false|true]},{is_stack[false|true]},{is_remove_background[false|true]},{speckles},{smooth_corners},{optimize}

# Example usage:
inkscape.exe --actions="select-all;selection-trace:256,false,true,true,4,1.0,0.20;export-filename:output.svg;export-do;" "input.png" --batch-process
```

**Parameters:**
- `scans`: Number of colors (int)
- `is_smooth`: Enable smooth tracing (bool: `true`/`false` or `1`/`0`)
- `is_stack`: Stack scan results (bool)
- `is_remove_background`: Remove background (bool)
- `speckles`: Speckle suppression size - `potraceParams->turdsize` (int)
- `smooth_corners`: Corner smoothing - `potraceParams->alphamax` (float)
- `optimize`: Path optimization tolerance - `potraceParams->opttolerance` (float)

The tracing uses the **Potrace** engine with `TRACE_QUANT_COLOR` mode.

## Architecture

### Document Model (MVC with split model)
- **XML Backbone** (`src/xml/`): Agnostic XML tree (`SPRepr`) - lightweight, typeless nodes
- **SVG Document** (`src/object/`): Typed object tree (`SPObject` subclasses) built on top of XML tree
- **View** (`src/display/`): NRArena display engine for rendering
- **Controllers** (`src/ui/tools/`): Event contexts for each tool

### Key Classes
- `SPDocument` (`src/document.h`): Container for both model trees, implements undo/redo
- `SPObject`: Abstract base class for SVG document nodes
- `SPItem`: Abstract base class for visible SVG elements
- `SPDesktop`: Editable document canvas view
- `InkscapeApplication` (`src/inkscape-application.h`): Main application singleton, handles CLI actions
- `InkscapeWindow`: GUI window containing an SPDesktop

### Entry Points
- `src/inkscape-main.cpp`: Main Inkscape entry point
- `src/inkview-main.cpp`: Inkview (SVG viewer) entry point

### Source Layout
- `src/actions/`: Gio::Actions for GUI-independent operations
- `src/extension/`: Extension system (internal + external Python)
- `src/io/`: File I/O operations
- `src/trace/`: Bitmap tracing (Potrace, Autotrace, Depixelize)
- `src/live_effects/`: Live Path Effects (LPE)
- `src/livarot/`: Path boolean operations library
- `src/3rdparty/`: Bundled libraries (2geom, autotrace, libdepixelize, etc.)

## Special Patterns

### Undo Implementation
Changes to the XML tree automatically generate undo records. Use:
- `sp_document_done()`: Push action list to undo stack
- `sp_document_maybe_done(key)`: Merge with previous if same key (for UI spinbutton-like operations)

For efficient undo during grab-drag-release UI operations, modify SPObject directly during drag, only write back to XML on release.

### Object Signals
SPObject uses asynchronous notification via `::modified` signal scheduled from idle loop. Flags indicate what changed.

### Submodules
Extensions are in a separate git submodule at `share/extensions/`. Update with:
```bash
git submodule update --remote
```

