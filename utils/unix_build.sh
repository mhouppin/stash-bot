#!/bin/sh

cd src

CFLAGS="-fprofile-generate" LDFLAGS="-lgcov" ARCH="$ARCH" make re

./stash-bot bench

CFLAGS="-fprofile-use -fno-peel-loops -fno-tracer" LDFLAGS="-lgcov" \
	ARCH="$ARCH" make re
