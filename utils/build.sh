#!/bin/sh

cd src

CFLAGS="$CFLAGS -fprofile-generate" LDFLAGS="$LDFLAGS -lgcov" ARCH="$ARCH" make re

./stash-bot bench

CFLAGS="$CFLAGS -fprofile-use -fno-peel-loops -fno-tracer" LDFLAGS="$LDFLAGS -lgcov" \
    ARCH="$ARCH" make re
