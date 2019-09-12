# macOS build pipeline

This folder contains the scripts that make up the build pipeline for Inkscape on macOS.

## Requirements

_These requirements have changed a few times over the course of development. So it would be more fair to call them "recommendations" instead, but I want to emphasize the importance of sticking to a known-good setup because of the huge number of moving parts involved._

- __A clean environment is key__. Ideally, you'd have a __dedicated, clean macOS installation__ (as in "freshly installed + Xcode") available as build machine.
  - Make sure there are no remnants from other build environments (e.g. MacPorts, Fink, Homebrew) on your system.  
    Rule of thumb: clear out `/usr/local`.
  - macOS 10.14.6 with Xcode 10.3.
  - OS X Mavericks 10.9 SDK from Xcode 6.4  
    `/Library/Developer/CommandLineTools/SDKs/MacOSX10.9.sdk`

- __Use a dedicated user account__ unless you're prepared that these scripts will delete and overwrite your data in the following locations:  
_(based on default configuration)_

    ```bash
    $HOME/.cache               # will be removed, then linked to $TMP_DIR
    $HOME/.config/jhbuildrc*   # will be removed, then linked to $DEVCONFIG
    $HOME/.local               # will be removed, then linked to $OPT_DIR
    ```

- __16 GiB RAM__, since we're using a 9 GiB ramdisk to build everything.
  - Using a ramdisk speeds up the process significantly and avoids wearing out your SSD.
  - You can choose to not use a ramdisk by overriding the configuration.

    ```bash
    echo "RAMDISK_ENABLE=false" > 021-vars-custom.sh
    ```

  - The build environment takes up ~6.1 GiB of disk space, the rest is buffer to be used during compilation and packaging. Subject to change and YMMV.

  - If you only want to build Inkscape and not the build environment itself, a 5 GiB ramdisk is sufficient. (Not all of the tarball's content is extracted.)

- somewhat decent __internet connection__ for all the downloads

## Usage

### standalone

You can either run all the executable scripts that have a numerical prefix (>100) yourself and in the given order, or, if you're feeling bold, use

```bash
./build_all.sh
```

to have everything run for you. If you are doing this the first time, my advice is to do it manually and step-by-step first to see if you're getting through all of it without errors.

### GitLab CI

> TODO: this section needs to be updated!

#### configuration example `.gitlab-runner/config.toml`

```toml
[[runners]]
  name = "<YOUR RUNNER'S NAME>"
  url = "https://gitlab.com/"
  token = "<YOUR TOKEN>"
  executor = "shell"
  builds_dir = "/Users/<YOUR DEDICATED USER>/work/builds"
  cache_dir = "/Users/<YOUR DEDICATED USER>/work/cache"
```

#### configuration example `.gitlab-ci.yml`

```yaml
buildmacos:
  before_script:
    - packaging/macos/build_toolset.sh
  script:
    - packaging/macos/build_inkscape.sh
```

## Status

This is based on [mibap](https://github.com/dehesselle/mibap) and still a work in progress.
