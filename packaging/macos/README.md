# macOS build pipeline

This folder contains the scripts that make up the build pipeline for Inkscape on macOS.

## Usage

### Recommendations

ℹ️ _These tend to change as development progresses (as they already have a few times) and I won't deny that there's usually more than one way to do something, but I can only support what I use myself. So feel free to experiment and deviate, but know that __it is dangerous to go alone! Take this 🗡️.___

- __A clean environment is key.__
  - Make sure there are no remnants from other build environments (e.g. MacPorts, Fink, Homebrew) on your system.
    - Rule of thumb: clear out `/usr/local`.
  - Use macOS Mojave 10.14.6 with Xcode 10.3.
  - Copy OS X Mavericks 10.9 SDK from Xcode 6.3 to `/Library/Developer/CommandLineTools/SDKs/MacOSX10.9.sdk`.

- __Use a dedicated user account__ to avoid any interference with the environment (e.g. no custom settings in `.profile`, `.bashrc`, etc.).

- A somewhat decent __internet connection__ for all the downloads.

### Building the toolset

ℹ️ _If you only want to build Inkscape and not the complete toolset, skip ahead to the next section!_

1. Clone this repository and `cd` into it.

   ```bash
   git clone https://github.com/dehesselle/mibap
   cd mibap
   ```

2. Specify a folder where all the action is going to take place. (Please avoid spaces in paths!)

   ```bash
   echo "TOOLSET_ROOT_DIR=$HOME/my_build_dir" > 015-customdir.sh
   ```

3. Build the toolset.

   ```bash
   ./build_toolset.sh
   ```

4. ☕ (Time to get a beverage - this will take a while!)

### Installing a pre-compiled toolset

ℹ️ _If you built the toolset yourself, skip ahead to the next section!_

ℹ️ _Using `/Users/Shared/work` as `TOOLSET_ROOT_DIR` is mandatory! (This is the default.)_

1. Clone this repository and `cd` into it.

   ```bash
   git clone https://github.com/dehesselle/mibap
   cd mibap
   ```

2. Install the toolset.

   ```bash
   ./install_toolset.sh
   ```

   You should know what that actually does:

   - download a disk image (about 1.6 GiB) to `/Users/Shared/work/repo`
   - mount the disk image to `/Users/Shared/work/1`
   - union-mount a ramdisk (2 GiB) to `/Users/Shared/work/1`

   The mounted volumes won't show up in the finder but you can see (and eject) them using `diskutil`.

### Building Inkscape

1. Build Inkscape.

   ```bash
   ./build_inkscape.sh
   ```

   Ultimately this will produce `/Users/Shared/work/1/artifacts/Inkscape.dmg`.

## GitLab CI

The intended usage in `.gitlab-ci.yml` is as follows:

```yaml
buildmacos:
  before_script:
    - packaging/macos/install_toolset.sh
  script:
    - packaging/macos/build_inkscape.sh
```

## Status

This is based on [mibap](https://github.com/dehesselle/mibap) and still a work in progress.
