#!/bin/zsh -x

# This script is mainly a helper for me to quickly generate release builds when
# necessary. For now, it produces all Windows 64-bit binaries and all Linux
# 32-bit and 64-bit binaries. Future contributors may use this to avoid all
# the command-line typing when creating assets for the repository.

set -e

version=36.0

cd $(dirname "$0")

cd ../src

for arch in generic x86-64 x86-64-popcnt x86-64-bmi2
do
    ext_arch=${arch/x86-64/x86_64}
    ext_arch=${ext_arch/generic/64}

    ARCH="$arch" CFLAGS="-fprofile-generate -O3 -flto" LDFLAGS="-lgcov" make re

    ./stash-bot bench

    ARCH="$arch" CFLAGS="-fprofile-use -fno-peel-loops -fno-tracer -O3 -flto" \
        LDFLAGS="-lgcov -static" make re EXE="stash-$version-linux-$ext_arch" \

    ARCH="$arch" CC=x86_64-w64-mingw32-gcc LDFLAGS="-static" make re \
        EXE="stash-$version-windows-$ext_arch.exe" \

    rm $(find sources \( -name "*.gcda" \) )
done

CFLAGS="-m32 -O3 -flto" make re EXE="stash-$version-linux-i386" ARCH=i386 LDFLAGS="-lgcov -static"

make clean

rm stash-bot
