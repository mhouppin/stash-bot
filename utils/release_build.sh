#!/bin/zsh -x

# This script is mainly a helper for me to quickly generate release builds when
# necessary. For now, it produces all Windows 64-bit binaries and all Linux
# 32-bit and 64-bit binaries. Future contributors may use this to avoid all
# the command-line typing when creating assets for the repository.

set -e

version=24.7

cd $(dirname "$0")

cd ../src

cat Makefile | sed 's/-Wall -Wextra -Werror/-w/g' > tmp.make

for arch in 64 x86-64 x86-64-modern x86-64-bmi2
do
    ext_arch=${arch/x86-64/x86_64}

    ARCH="$arch" CFLAGS="-fprofile-generate" LDFLAGS="-lgcov" make -f tmp.make re native=no

    ./stash-bot bench

    ARCH="$arch" CFLAGS="-fprofile-use -fno-peel-loops -fno-tracer" LDFLAGS="-lgcov" \
        make -f tmp.make re EXE="stash-$version-linux-$ext_arch" native=no \

    ARCH="$arch" CC=x86_64-w64-mingw32-gcc CFLAGS="-fprofile-use -fno-peel-loops -fno-tracer" \
        LDFLAGS="-lgcov -static" make -f tmp.make re \
        EXE="stash-$version-windows-$ext_arch.exe" native=no \

    rm $(find sources \( -name "*.gcda" \) )
done

CFLAGS="-m32" make -f tmp.make re EXE="stash-$version-linux-i386" native=no

make -f tmp.make clean

rm stash-bot
rm -f tmp.make
