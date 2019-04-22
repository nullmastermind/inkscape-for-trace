# macOS build pipeline

This folder contains the scripts that make up the build pipeline for Inkscape on macOS.

## Requirements

_It would've been more fair if this section was called "recommandations" instead, but that would only encourage carelessness._

- Use a __dedicated, clean macOS installation__ as build machine. "clean" as in "freshly installed + Xcode CLI tools". Nothing more, nothing less.
  - Especially no MacPorts, no Fink, no Homebrew, ... because they could interfere with the build system we're using.
  - macOS 10.13.6 with Xcode 10.1.
    - These scripts are being developed and used using said versions.
    - Newer versions might work but haven't been tested.
  - A full Xcode installation won't hurt, but is not required.
- __Use a dedicated user account__ unless you're prepared that these scripts will delete and overwrite your data in the following locations:  
_(comments based on default configuration)_

    ```bash
    $HOME/.cache       # will be symlinked to $WRK_DIR/tmp
    $HOME/.local       # will be symlinked to $WRK_DIR/opt
    $HOME/.profile     # will be overwritten
    ```

- __16 GiB RAM__, since we're using a 10 GiB ramdisk to build everything.
  - Using a ramdisk speeds up the process significantly and avoids wearing out your ssd.
  - You can choose to not use a ramdisk by overriding `RAMDISK_ENABLE=false` in a e.g. `021-custom.sh` file.
  - The build environment takes up ~6.1 GiB of disk space, the Inkscape Git repository ~1.6 GiB. Subject to change and YMMV.
- somewhat decent __internet connection__ for all the downloads

## Usage

### standalone

> TODO: These scripts were initially designed as part of a dedicated repository, not the Inkscape repository itself. Therefore they contain a step that shallow-clones the Inkscape repository to build. This behaviour will be improved upon in a future version.

Run:

```bash
./build_everything.sh
```

This will run all the `nnn-*.sh` scripts consecutively.

### GitLab CI

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

This is still a work in progress and actively being worked on.
