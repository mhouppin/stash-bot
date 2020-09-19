#!/bin/sh

cd src

make re EXT_OFLAGS="-fprofile-generate" EXT_LFLAGS="-lgcov -static" ARCH="$ARCH"

./stash-bot bench

rm $(find sources \( -name "*.o" \) )

make EXT_OFLAGS="-fprofile-use -fno-peel-loops -fno-tracer -flto" \
	EXT_LFLAGS="-fprofile-use -fno-peel-loops -fno-tracer -lgcov -flto -static" \
	ARCH="$ARCH"
