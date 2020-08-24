#!/bin/sh

cd src

make re EXT_OFLAGS="-fprofile-generate" EXT_LFLAGS="-lgcov" ARCH="$ARCH"

./stash-bot bench

rm $(find objects \( -name "*.o" \) )

make EXT_OFLAGS="-fprofile-use -fno-peel-loops -fno-tracer -flto" \
	EXT_LFLAGS="-fprofile-use -fno-peel-loops -fno-tracer -lgcov -flto" \
	ARCH="$ARCH"
