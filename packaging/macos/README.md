# macOS build pipeline

This folder contains the scripts that make up the build pipeline for Inkscape on macOS.

## Requirements

_In some regards it would've been more fair if this section was called "recommendations" instead, but that would only encourage carelessness or deviating from a known-good setup._

- Use a __dedicated, clean macOS installation__ as build machine. "clean" as in "freshly installed + Xcode". Nothing more, nothing less.
  - Especially no MacPorts, no Fink, no Homebrew, ... because they could interfere with the build system we're using.
  - macOS 10.11.6 with Xcode 8.2.1.
  - macOS 10.11 SDK from Xcode 7.3.1
    - Place it inside your `Xcode.app` and re-symlink to make look as follows:

      ```bash
      elcapitan:SDKs rene$ pwd
      /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs
      elcapitan:SDKs rene$ ls -lah
      total 8
      drwxr-xr-x  5 root  wheel   170B 20 Jun 12:23 .
      drwxr-xr-x  5 root  wheel   170B 15 Dez  2016 ..
      lrwxr-xr-x  1 root  wheel    15B 20 Jun 12:23 MacOSX.sdk -> MacOSX10.11.sdk
      drwxr-xr-x  5 root  wheel   170B 21 Okt  2020 MacOSX10.11.sdk
      drwxr-xr-x  5 root  wheel   170B 17 Sep  2017 MacOSX10.12.sdk.disabled
      ```

- __Use a dedicated user account__ unless you're prepared that these scripts will delete and overwrite your data in the following locations:  
_(based on default configuration)_

    ```bash
    $HOME/.cache               # will be linked to $TMP_DIR
    $HOME/.config/jhbuildrc*   # will be overwritten
    $HOME/.local               # will be linked to $OPT_DIR
    $HOME/.profile             # will be overwritten
    ```

- __16 GiB RAM__, since we're using a 9 GiB ramdisk to build everything.
  - Using a ramdisk speeds up the process significantly and avoids wearing out your SSD.
  - You can choose to not use a ramdisk by overriding `RAMDISK_ENABLE=false` in a e.g. `021-custom.sh` file.
  - The build environment takes up ~6.1 GiB of disk space, the Inkscape Git repository ~1.6 GiB. Subject to change and YMMV.
- somewhat decent __internet connection__ for all the downloads

## Usage

### standalone

> TODO: These scripts were initially designed as part of a dedicated repository, not the Inkscape repository itself. Therefore they contain a step that shallow-clones the Inkscape repository to build. This behaviour will be improved upon in a future version.

You can either run all the executable scripts that have a numerical prefix yourself and in the given order (`1nn`, `2nn` - not the `0nn` ones), or, if you're feeling bold, use

```bash
./build_all.sh
```

to have everything run for you. If you are doing this the first time, my advice is to do it manually and step-by-step first to see if you're getting through all of it without errors.

### GitLab CI

> TODO: configuration examples need to be updated!

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
