#!/bin/zsh -x

# This script is mainly a helper for me to quickly generate release builds when
# necessary. For now, it produces all Windows 64-bit binaries and all Linux
# 32-bit and 64-bit binaries. Future contributors may use this to avoid all
# the command-line typing when creating assets for the repository.

set -e

version=15.4

cd src

cat Makefile | sed 's/-Werror/-w/g' > tmp.make

for build_arch in 64 x86-64 x86-64-modern x86-64-bmi2
do
	ext_arch=${build_arch/x86-64/x86_64}

	make -f tmp.make re EXT_OFLAGS="-fprofile-generate" EXT_LFLAGS="-lgcov" \
		ARCH="$build_arch"

	./stash-bot bench

	rm -f stash-bot
	rm $(find objects \( -name "*.o" \) )

	make -f tmp.make EXE="stash-$version-linux-$ext_arch" \
		EXT_OFLAGS="-fprofile-use -fno-peel-loops -fno-tracer -flto" \
		EXT_LFLAGS="-lgcov" ARCH="$build_arch"

	rm $(find objects \( -name "*.o" \) )

	CC=x86_64-w64-mingw32-gcc make -f tmp.make EXE="stash-$version-windows-$ext_arch.exe" \
		EXT_OFLAGS="-fprofile-use -fno-peel-loops -fno-tracer -flto" \
		EXT_LFLAGS="-lgcov -static" ARCH="$build_arch"
done

make -f tmp.make re EXT_OFLAGS="-fprofile-generate -m32" EXT_LFLAGS="-lgcov" \

./stash-bot bench

rm -f stash-bot
rm $(find objects \( -name "*.o" \) )

make -f tmp.make EXE="stash-$version-linux-i386" \
	EXT_OFLAGS="-fprofile-use -fno-peel-loops -fno-tracer -flto -m32" \
	EXT_LFLAGS="-lgcov"

make -f tmp.make clean

rm -f tmp.make
