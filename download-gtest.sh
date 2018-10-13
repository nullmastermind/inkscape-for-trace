#!/bin/bash
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
    if false
    then
        mock_ver=1.7.0
        if ! test -e gmock-$gmock_ver ; then
            wget -O "googlemock-release-$gmock_ver.tar.gz" https://github.com/google/googlemock/archive/release-$gmock_ver.tar.gz
            tar -xvf "googlemock-release-$gmock_ver.tar.gz"
            mv googlemock-release-$gmock_ver gmock-$gmock_ver
        fi
    fi
)
