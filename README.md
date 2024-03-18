
## Stash

Stash is a UCI-compliant chess engine developed from scratch. Preferably used
with a GUI like Nibbler, CuteChess, Arena, etc.

## UCI Parameters

Stash supports for now all these UCI options:

  * #### Threads
    Sets the number of cores used for searching a position (defaults to 1).

  * #### Hash
    Sets the hash table size in MB (defaults to 16).

  * #### Clear Hash
    Clears the hash table.

  * #### MultiPV
    Output the best N lines (principal variations) when searching.
    Leave at 1 for best performance.

  * #### Move Overhead
    Assumes a time delay of x milliseconds due to network and GUI overheads.
    Increase it if the engine often loses games on time. The default value
    of 100 ms should be sufficient for all chess GUIs.

  * #### NormalizeScore
    Normalizes the search score so that a 100 centipawns advantage corresponds
    to a win rate of ~50% at move 32. Enabled by default.

  * #### UCI\_ShowWDL
    Displays the expected probabilities of win/draw/loss per mill, alongside
    the search score. Only enable it if your GUI supports it.

## Frequently Asked Questions

  * #### How do I compile this project for my computer ?
    If you are on a Unix-like system, simply go to the src directory from a
    terminal and type:
    ```
    make
    ```
    The Makefile normally autodetects which arch-specific options are the most
    suitable for the host system you're compiling on. Specific architecture
    optimizations may be applied by using:
    ```
    make ARCH=arch_name
    ```
    with `arch_name` being one of the following: x86-64, x86-64-popcnt or
    x86-64-bmi2. For non-x86 builds, or 32-bit x86 builds, you can use
    `ARCH=generic` instead. Note that specifying the arch manually is only
    required if you're building the engine for a different host, or if the
    Makefile fails to detect properly the host CPU.

    Additionally, for native binaries you can also pass `NATIVE=yes` to the
    Makefile to enable the usage of all available instruction sets on the host.

  * #### I do not have a compiler on my machine: how do I do ?
    Compiled binaries for Linux and Windows are available from the "releases"
    page of the project. You can download the binary corresponding to your
    operating system and architecture.

  * #### Which binary should I choose ?
    The latest release should always be the best. The different architecture
    builds are:
      - 64: generic 64-bit build. Should work on all x86_64 processors.

      - x86_64: x86_64 build. The only thing specific to this binary is the
        usage of the `prefetch` instruction, which places RAM addresses in
        cache for faster access. Should work on almost all x86_64 processors
        (except for a few old Xeon Phis).

      - x86_64-popcnt: same as previous one, but also enables use of the
        `popcnt` (Population Count) instruction. Should work on all K10-based
        AMD processors or newer, and all Intel Nehalem processors or newer.

      - x86_64-bmi2: same as previous one, but also enables use of the `pext`
        (Parallel Bit Extract) instruction. Should work on all AMD
        processors with Excavator arch or newer, and all Intel processors with
        Haswell arch or newer. Note that you should avoid using this binary for
        Zen-based AMD processors which are not Zen 3 or newer, as the `pext`
        microcode implementation will make the bmi2 binary slower than the
        popcnt one.
