# macOS build pipeline

This folder contains the scripts that make up the build pipeline for Inkscape on macOS.

## Usage

### Requirements

ℹ️ _The following is bound to change as development progresses and I won't deny that there's usually more than one way to do something, but I can only support what I use myself. So feel free to experiment and deviate, but know that __it is dangerous to go alone! Take this 🗡️.___

- A __clean environment__ is key.
  - Make sure there are no remnants from other build environments (e.g. MacPorts, Fink, Homebrew) on your system.
    - Rule of thumb: clear out `/usr/local`.
  - Use a dedicated user account to avoid any interference with the environment (e.g. no custom settings in `.profile`, `.bashrc`, etc.).

- There are __version recommendations__.
  - macOS Catalina 10.15.6
  - Xcode 11.6
  - OS X El Capitan 10.11 SDK from Xcode 7.3.1 (expected in `/opt/sdks/MacOSX10.11.sdk`)

- A somewhat decent __internet connection__ for all the downloads.

### Building the toolset

ℹ️ _If you only want to build Inkscape and not the complete toolset, skip ahead to the next section!_

1. Clone this repository and `cd` into `packaging/macos`.

   ```bash
   git clone --depth 1 https://gitlab.com/inkscape/inkscape
   cd inkscape/packaging/macos
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
   git clone --depth 1 https://gitlab.com/inkscape/inkscape
   cd inkscape/packaging/macos
   ```

2. Install the toolset.

   ```bash
   ./install_toolset.sh
   ```

   This will

   - download a disk image (about 1.6 GiB) to `/Users/Shared/work/repo`
   - mount the disk image to `/Users/Shared/work/$TOOLSET_VERSION`
   - union-mount a ramdisk (2 GiB) to `/Users/Shared/work/$TOOLSET_VERSION`

   The mounted volumes won't show up in the Finder but you can see them using `diskutil`. Use `uninstall_toolset.sh` to eject them (this won't delete `repo` though).

### Building Inkscape

1. Build Inkscape.

   ```bash
   ./build_inkscape.sh
   ```

   Ultimately this will produce `/Users/Shared/work/$TOOLSET_VERSION/artifacts/Inkscape.dmg`.

## GitLab CI

Make sure the runner fulfills the same requirements as listed in the usage section above.

Configure a job in your `.gitlab-ci.yml` as follows:

```yaml
buildmacos:
  before_script:
    - packaging/macos/install_toolset.sh
  script:
    - packaging/macos/build_inkscape.sh
  after_script:
    - packaging/macos/uninstall_toolset.sh
  artifacts:
    paths:
      - artifacts/
```

## Status

This is based on [mibap](https://github.com/dehesselle/mibap) and still a work in progress.
