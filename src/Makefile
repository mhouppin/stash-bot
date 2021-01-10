#
#    Stash, a UCI chess playing engine developed from scratch
#    Copyright (C) 2019-2020 Morgan Houppin
#
#    Stash is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Stash is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

SOURCES := $(wildcard sources/*/*.c)

OBJECTS := $(SOURCES:%.c=%.o)
DEPENDS := $(SOURCES:%.c=%.d)
native = no

CFLAGS += -Wall -Wextra -Werror -O3 -flto
CPPFLAGS += -MMD -I include
LDFLAGS += -lpthread -lm

# If no arch is specified, we select a prefetch+popcnt build by default.
# If you want to build on a different architecture, please specify it in the
# command with ARCH=something. If you want to disable all arch-specific
# optimizations, use ARCH=unknown.

ifeq ($(ARCH),)
	ARCH=x86-64-modern
endif

# Check if we can accelerate a bit the build process when using -jXX

compiler_version = $(shell $(CC) --version)

# Add .exe to the executable name if we are on Windows

EXE=stash-bot
ifeq ($(OS),Windows_NT)
	EXE=stash-bot.exe
endif

# Enable use of PREFETCH instruction

ifeq ($(ARCH),x86-64)
    CFLAGS += -DUSE_PREFETCH
    ifneq ($(native),yes)
        CFLAGS += -msse
    endif
endif

ifeq ($(ARCH),x86-64-modern)
    CFLAGS += -DUSE_PREFETCH -DUSE_POPCNT
    ifneq ($(native),yes)
        CFLAGS += -msse -msse3 -mpopcnt
    endif
endif

ifeq ($(ARCH),x86-64-bmi2)
    CFLAGS += -DUSE_PREFETCH -DUSE_POPCNT -DUSE_PEXT
    ifneq ($(native),yes)
        CFLAGS += -msse -msse3 -mpopcnt -msse4 -mbmi2
    endif
endif

# If native is specified, build will try to use all available CPU instructions

ifeq ($(native),yes)
    CFLAGS += -march=native
endif

all: $(EXE)

$(EXE): $(OBJECTS)
	+$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

-include $(DEPENDS)

clean:
	rm -f $(OBJECTS) $(DEPENDS)

fclean: clean
	rm -f $(EXE)

re: fclean all

.PHONY: all clean fclean re
