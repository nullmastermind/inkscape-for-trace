# SPDX-License-Identifier: GPL-2.0-or-later

### settings ###################################################################

# shellcheck shell=bash # no shebang as this file is intended to be sourced

### description ################################################################

# Install Mozilla root certificates to facilitate SSL certificate checks.

### variables ##################################################################

# https://pypi.org/project/certifi/
CERTIFI_PIP=certifi   # unversioned on purpose

### functions ##################################################################

function certifi_install
{
  pip3 install --prefix "$VER_DIR" "$CERTIFI_PIP"
}
