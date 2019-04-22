#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of the build pipeline for Inkscape on macOS.
#
# ### 130-jhbuild-python.sh ###
# Install Python 2 and 3.

### load settings and functions ################################################

SELF_DIR=$(cd $(dirname "$0"); pwd -P)
for script in $SELF_DIR/0??-*.sh; do source $script; done

### install OpenSSL ############################################################

# Build OpenSSL as dedicated step because we need to link our system
# configuration and certs (/etc/ssl) to it. Otherwise https downloads
# will fail with certification validation issues.

jhbuild build openssl
mkdir -p $OPT_DIR/etc
ln -sf /etc/ssl $OPT_DIR/etc   # link system config to our OpenSSL

### install Python 2 ###########################################################

# Some packages complain about non-exiting development headers when you rely
# solely on the macOS-provided Python installation. This also enables
# system-wide installations of packages without permission issues.

jhbuild build python

cd $SRC_DIR
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
jhbuild run python get-pip.py
jhbuild run pip install six   # required for a package in meta-gtk-osx-bootstrap

### install Python 3 ###########################################################

# Since enabling the Framework build ('--enable-framework'), 'install' fails
# on multi-threaded build ('$MAKEFLAGS')

unset MAKEFLAGS   # 'install' step would fail otherwise

jhbuild build python3

# make library link paths relative
install_name_tool -change $OPT_DIR/Frameworks/Python.framework/Versions/3.6/Python @executable_path/../../../Versions/3.6/Python $OPT_DIR/Frameworks/Python.framework/Versions/3.6/bin/python3.6
install_name_tool -change $OPT_DIR/Frameworks/Python.framework/Versions/3.6/Python @executable_path/../../../Versions/3.6/Python $OPT_DIR/Frameworks/Python.framework/Versions/3.6/bin/python3.6m
install_name_tool -change $OPT_DIR/Frameworks/Python.framework/Versions/3.6/Python @executable_path/../../../../Python $OPT_DIR/Frameworks/Python.framework/Versions/3.6/Resources/Python.app/Contents/MacOS/Python

# replace hard-coded interpreter path in shebang with an environment lookup
PYTHON_BIN_DIR=$OPT_DIR/Frameworks/Python.framework/Versions/3.6/bin
sed -i "" "1s/.*/#!\/usr\/bin\/env python3.6/" $PYTHON_BIN_DIR/2to3-3.6
sed -i "" "1s/.*/#!\/usr\/bin\/env python3.6/" $PYTHON_BIN_DIR/easy_install-3.6
sed -i "" "1s/.*/#!\/usr\/bin\/env python3.6/" $PYTHON_BIN_DIR/idle3.6
sed -i "" "1s/.*/#!\/usr\/bin\/env python3.6/" $PYTHON_BIN_DIR/pip3
sed -i "" "1s/.*/#!\/usr\/bin\/env python3.6/" $PYTHON_BIN_DIR/pip3.6
sed -i "" "1s/.*/#!\/usr\/bin\/env python3.6/" $PYTHON_BIN_DIR/pydoc3.6
sed -i "" "1s/.*/#!\/usr\/bin\/env python3.6/" $PYTHON_BIN_DIR/python3.6m-config
sed -i "" "1s/.*/#!\/usr\/bin\/env python3.6/" $PYTHON_BIN_DIR/pyvenv-3.6
