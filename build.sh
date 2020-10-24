#!/bin/bash
set -e
for abi in armeabi-v7a "armeabi-v7a with NEON" arm64-v8a x86 x86_64;
do
    rm -rf "$abi"
    mkdir "$abi"
    cd "$abi"
    cmake -G "Unix Makefiles" \
        -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
        -DANDROID_NDK=$NDK \
        -DCMAKE_MAKE_PROGRAM=$NDK/prebuilt/windows-x86_64/bin/make.exe \
        -DCMAKE_BUILD_TYPE=Release \
        -DANDROID_ABI="$abi" \
        ..
    $NDK/prebuilt/windows-x86_64/bin/make.exe -j5 tun2call
    cd ..
done
