<div align="center">
    <h1><code>Inkscape</code></h1>
    <p><strong>Used only for automation scripts</strong></p>
</div>

## <code>Features</code>

- [x] Add options for tracing

This is a video where I used this repository to automate some pixel art image editing: https://www.youtube.com/watch?v=-rXxJMkYn5w

## <code>Quick guide</code>

```bash
# New trace action with fully optional parameters
selection-trace:{scans},{is_smooth[false|true]},{is_stack[false|true]},{is_remove_background[false|true],{speckles},{smooth_corners},{optimize}}
# Example:
# Trace 256 colors then export to output.svg
$ inkscape.exe --actions="select-all;selection-trace:256,false,true,true,4,1.0,0.20;export-filename:output.svg;export-do;" "input.png" --batch-process

# If "false|true" does not work, please replace it with "0|1", because my project has been bankrupt for a long time and is no longer maintained, but rest assured that it still works very well.
```

If you encounter the "png bitmap image import" settings popup, please follow this issue to disable it: https://github.com/nullmastermind/inkscape-for-trace/issues/2

---

## <code>Linux (Ubuntu/Debian)</code>

This fork includes scripts for building and running headless tracing on Linux (Ubuntu, Debian, or WSL).

### Prerequisites

- Ubuntu, Debian, or WSL Ubuntu/Debian environment
- `sudo` privileges (for dependency installation)
- `git` (must be pre-installed)

### Building from Source

Use the provided `release.sh` script for a one-command build:

```bash
# Clone the repository
git clone --recurse-submodules https://github.com/nullmastermind/inkscape-for-trace.git
cd inkscape-for-trace

# Run the release build script (installs dependencies, configures, and builds)
./release.sh
```

**What `release.sh` does:**

1. Detects the environment (Ubuntu/Debian/WSL)
2. Installs all required build dependencies via `apt-get`:
   - Core build tools: `build-essential`, `cmake`, `ninja-build`, `intltool`, `pkg-config`, `ccache`, `dos2unix`
   - Libraries: `libboost-dev`, `libgtk-3-dev`, `libgtkmm-3.0-dev`, `libpotrace-dev`, `libpoppler-glib-dev`, `libxml2-dev`, and more
   - Python packages: `python3-lxml`, `python3-numpy`, `python3-pil`
3. Initializes git submodules
4. Fixes CRLF line endings (important for Windows-cloned repos)
5. Configures with CMake (Ninja preferred, falls back to Make)
6. Builds a Release configuration
7. Installs artifacts to `release-output/`

**Build output locations:**

- Binary (build): `./build/bin/inkscape`
- Binary (installed): `./release-output/bin/inkscape`

**Environment variables:**

- `RELEASE_DEBUG=1` — Enable verbose debug output during build

### Headless Tracing Workflow

Use the provided `trace.sh` script for headless bitmap tracing:

```bash
# Basic usage (uses input.png -> output.svg by default)
./trace.sh

# Custom input/output
./trace.sh myimage.png result.svg
```

**`trace.sh` parameters:**

```bash
./trace.sh [input.png] [output.svg]
```

The script runs:

```bash
./build/bin/inkscape --headless --actions="select-all;selection-trace:256,false,true,true,2,1.0,0.20;export-filename:${OUTPUT};export-do;" "${INPUT}"
```

**Key flags:**

- `--headless` — Runs without GUI, quits after processing (suitable for CI/automation)
- `--actions="..."` — Executes the tracing pipeline

### Manual Tracing Command

For custom parameters, run directly:

```bash
./build/bin/inkscape --headless \
  --actions="select-all;selection-trace:256,false,true,true,4,1.0,0.20;export-filename:output.svg;export-do;" \
  input.png
```

**`selection-trace` parameters:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `scans` | int | Number of colors (e.g., `256`) |
| `is_smooth` | bool | Enable smooth tracing (`true`/`false` or `1`/`0`) |
| `is_stack` | bool | Stack scan results |
| `is_remove_background` | bool | Remove background |
| `speckles` | int | Speckle suppression size (Potrace `turdsize`) |
| `smooth_corners` | float | Corner smoothing (Potrace `alphamax`) |
| `optimize` | float | Path optimization tolerance (Potrace `opttolerance`) |

### Alternative: Manual Build Steps

If you prefer manual control over the build process:

```bash
# 1. Install dependencies (see release.sh for full list, or run it once)
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build intltool pkg-config \
  libboost-dev libgtk-3-dev libgtkmm-3.0-dev libpotrace-dev libpoppler-glib-dev \
  libxml2-dev libxslt1-dev libgc-dev libgsl-dev ccache

# 2. Initialize submodules
git submodule update --init --recursive

# 3. Create build directory and configure
mkdir -p build && cd build
cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=../release-output \
  -DCMAKE_C_COMPILER_LAUNCHER=ccache \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  ..

# 4. Build
ninja

# 5. (Optional) Install to release-output/
ninja install
```

### Troubleshooting

- **CRLF line ending errors**: If building from a Windows-cloned repo, run `dos2unix` on build scripts or use `release.sh` which handles this automatically
- **Missing dependencies**: Run `./release.sh` once to install all required packages
- **Permission denied on scripts**: Run `chmod +x release.sh trace.sh`

---

For Windows/macOS builds, see [INSTALL.md](INSTALL.md) or the [Inkscape wiki](https://wiki.inkscape.org/wiki/index.php/CompilingInkscape).
