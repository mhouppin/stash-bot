#!/bin/sh

cd src

make re CFLAGS="-O3 -flto -fprofile-generate" LDFLAGS="-lgcov" ARCH="$ARCH"

./stash bench

make re CFLAGS="-O3 -flto -fprofile-use -fno-peel-loops -fno-tracer" LDFLAGS="-lgcov" ARCH="$ARCH"

rm sources/*.gcda
