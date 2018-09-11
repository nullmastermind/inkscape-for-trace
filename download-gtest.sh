#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
if ! test -e gtest ; then
    mkdir gtest
fi
(
    cd gtest
    if ! test -e gtest ; then
        gtest_ver=1.8.1
        wget -O "googletest-release-$gtest_ver.tar.gz" https://github.com/google/googletest/archive/release-$gtest_ver.tar.gz
        tar -xvf "googletest-release-$gtest_ver.tar.gz"
        mv googletest-release-$gtest_ver gtest
    fi
)
